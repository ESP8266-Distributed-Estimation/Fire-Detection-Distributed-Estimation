package main

import (
	"encoding/json"
	"fmt"
	"log"
	"net/http"
	"sync"
	"time"

	mqtt "github.com/eclipse/paho.mqtt.golang"
	"github.com/gorilla/websocket"
	"github.com/rs/cors"
)

type SensorNode struct {
	ID     int    `json:"id"`
	Scheme string `json:"scheme"`
}

// Data structure received from the Gateway's MQTT payload
type EdgeStatus struct {
	EdgeNodeID     int               `json:"edge_node_id"`
	Status         string            `json:"status"`
	Uptime         int               `json:"uptime"`
	ConnectedNodes []SensorNode      `json:"connected_nodes"`
	FiringNodes    []int             `json:"firing_nodes"`
	LastSeen       time.Time         `json:"-"`
	Alarms         map[int]time.Time `json:"-"`
}

var (
	// Store state for all gateways
	state      = make(map[int]*EdgeStatus)
	stateMutex sync.RWMutex

	// Store connected WebSocket clients
	clients      = make(map[*websocket.Conn]bool)
	clientsMutex sync.Mutex

	mqttClient mqtt.Client

	// WebSocket upgrader
	upgrader = websocket.Upgrader{
		CheckOrigin: func(r *http.Request) bool { return true }, // Allow all origins for dev
	}
)

func main() {
	// Configure MQTT Client
	opts := mqtt.NewClientOptions().AddBroker("tcp://localhost:1883")
	opts.SetClientID("lbp-go-backend")

	opts.SetDefaultPublishHandler(func(client mqtt.Client, msg mqtt.Message) {
		topic := msg.Topic()
		if topic == "fire_alarm/edge_status" {
			var status EdgeStatus
			err := json.Unmarshal(msg.Payload(), &status)
			if err != nil {
				log.Printf("Error unmarshaling MQTT payload: %v", err)
				return
			}

			status.LastSeen = time.Now()

			// Update internal state
			stateMutex.Lock()
			if existing, ok := state[status.EdgeNodeID]; ok {
				status.Alarms = existing.Alarms
			} else {
				status.Alarms = make(map[int]time.Time)
			}
			state[status.EdgeNodeID] = &status
			stateMutex.Unlock()

			// Broadcast updated state to all UI clients
			broadcastState()
		} else if topic == "fire_alarm/alerts" {
			var alarm struct {
				EdgeNodeID   int `json:"edge_node_id"`
				FireSourceID int `json:"fire_source_id"`
			}
			err := json.Unmarshal(msg.Payload(), &alarm)
			if err != nil {
				log.Printf("Error unmarshaling alarm payload: %v", err)
				return
			}

			stateMutex.Lock()
			if gw, ok := state[alarm.EdgeNodeID]; ok {
				if gw.Alarms == nil {
					gw.Alarms = make(map[int]time.Time)
				}
				gw.Alarms[alarm.FireSourceID] = time.Now()
			}
			stateMutex.Unlock()

			broadcastState()
		} else if topic == "fire_alarm/scheme_ack" {
			var ack struct {
				GatewayID int    `json:"gateway_id"`
				NodeID    int    `json:"node_id"`
				Scheme    string `json:"scheme"`
			}
			err := json.Unmarshal(msg.Payload(), &ack)
			if err != nil {
				return
			}
			
			log.Printf("[SCHEME ACK] Sensor Node %d on Gateway %d confirmed shift to '%s' scheme\n", ack.NodeID, ack.GatewayID, ack.Scheme)
			
			stateMutex.Lock()
			if gw, ok := state[ack.GatewayID]; ok {
				for i, node := range gw.ConnectedNodes {
					if node.ID == ack.NodeID {
						gw.ConnectedNodes[i].Scheme = ack.Scheme
						break
					}
				}
			}
			stateMutex.Unlock()
			broadcastState()
		}
	})

	opts.OnConnect = func(c mqtt.Client) {
		log.Println("Connected to MQTT Broker")
		// Subscribe to the topics
		if token := c.Subscribe("fire_alarm/edge_status", 0, nil); token.Wait() && token.Error() != nil {
			log.Fatalf("Error subscribing to edge_status: %v", token.Error())
		}
		if token := c.Subscribe("fire_alarm/alerts", 0, nil); token.Wait() && token.Error() != nil {
			log.Fatalf("Error subscribing to alerts: %v", token.Error())
		}
		if token := c.Subscribe("fire_alarm/scheme_ack", 0, nil); token.Wait() && token.Error() != nil {
			log.Fatalf("Error subscribing to scheme_ack: %v", token.Error())
		}
		log.Println("Subscribed to MQTT topics")
	}

	opts.OnConnectionLost = func(c mqtt.Client, err error) {
		log.Printf("Lost MQTT connection: %v", err)
	}

	// Connect to MQTT
	mqttClient = mqtt.NewClient(opts)
	if token := mqttClient.Connect(); token.Wait() && token.Error() != nil {
		log.Fatalf("Failed to connect to MQTT broker: %v\nMake sure your Mosquitto broker is running on localhost:1883", token.Error())
	}
	defer mqttClient.Disconnect(250)

	// Clean up stale gateways and alarms periodically
	go func() {
		for {
			time.Sleep(2 * time.Second)
			stateMutex.Lock()
			changed := false
			now := time.Now()
			for id, s := range state {
				if now.Sub(s.LastSeen) > 10*time.Second {
					delete(state, id)
					changed = true
				} else {
					// Clean up stale alarms (if no new alarm in 15 seconds)
					for nodeID, lastAlarm := range s.Alarms {
						if now.Sub(lastAlarm) > 15*time.Second {
							delete(s.Alarms, nodeID)
							changed = true
						}
					}
				}
			}
			stateMutex.Unlock()
			if changed {
				broadcastState()
			}
		}
	}()

	// Setup HTTP/WS Server
	mux := http.NewServeMux()
	mux.HandleFunc("/ws", handleWebSocket)
	mux.HandleFunc("/api/state", handleStateHttp) // fallback if UI needs initial state
	mux.HandleFunc("/api/set_scheme", handleSetScheme)

	handler := cors.Default().Handler(mux)
	log.Println("Go Backend listening on :8080 (WS on /ws)")
	log.Fatal(http.ListenAndServe(":8080", handler))
}

func handleSetScheme(w http.ResponseWriter, r *http.Request) {
	if r.Method != http.MethodPost {
		http.Error(w, "Method not allowed", http.StatusMethodNotAllowed)
		return
	}
	var req struct {
		GatewayID int    `json:"gateway_id"`
		Scheme    string `json:"scheme"`
	}
	if err := json.NewDecoder(r.Body).Decode(&req); err != nil {
		http.Error(w, err.Error(), http.StatusBadRequest)
		return
	}

	topic := fmt.Sprintf("fire_alarm/command/%d", req.GatewayID)
	msg, _ := json.Marshal(map[string]string{"command": "set_scheme", "scheme": req.Scheme})
	if token := mqttClient.Publish(topic, 0, false, msg); token.Wait() && token.Error() != nil {
		http.Error(w, token.Error().Error(), http.StatusInternalServerError)
		return
	}

	w.WriteHeader(http.StatusOK)
}

func broadcastState() {
	stateMutex.RLock()
	// Create a list from the map to send
	var stateList []*EdgeStatus
	for _, s := range state {
		// Populate FiringNodes array dynamically based on active Alarms
		s.FiringNodes = make([]int, 0, len(s.Alarms))
		for nodeID := range s.Alarms {
			s.FiringNodes = append(s.FiringNodes, nodeID)
		}
		stateList = append(stateList, s)
	}
	stateMutex.RUnlock()

	data, err := json.Marshal(stateList)
	if err != nil {
		log.Printf("Error marshaling state for broadcast: %v", err)
		return
	}

	clientsMutex.Lock()
	for client := range clients {
		err := client.WriteMessage(websocket.TextMessage, data)
		if err != nil {
			log.Printf("WS client error: %v", err)
			client.Close()
			delete(clients, client)
		}
	}
	clientsMutex.Unlock()
}

func handleWebSocket(w http.ResponseWriter, r *http.Request) {
	conn, err := upgrader.Upgrade(w, r, nil)
	if err != nil {
		log.Printf("Failed to upgrade WS: %v", err)
		return
	}

	clientsMutex.Lock()
	clients[conn] = true
	clientsMutex.Unlock()

	// Send initial state upon connection
	broadcastState()

	// Wait for disconnect
	for {
		_, _, err := conn.ReadMessage()
		if err != nil {
			clientsMutex.Lock()
			delete(clients, conn)
			clientsMutex.Unlock()
			conn.Close()
			break
		}
	}
}

func handleStateHttp(w http.ResponseWriter, r *http.Request) {
	stateMutex.RLock()
	var stateList []*EdgeStatus
	for _, s := range state {
		stateList = append(stateList, s)
	}
	stateMutex.RUnlock()

	w.Header().Set("Content-Type", "application/json")
	json.NewEncoder(w).Encode(stateList)
}
