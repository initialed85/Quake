package main

import (
	"bytes"
	"flag"
	"fmt"
	_log "log"
	"maps"
	"net"
	"net/http"
	"os"
	"runtime"
	"slices"
	"sync"
	"time"

	"github.com/gorilla/websocket"
)

type Addr struct {
	IP   string
	Port int
}

func (a *Addr) String() string {
	return fmt.Sprintf("%s:%d", a.IP, a.Port)
}

var log = _log.New(os.Stdout, "", _log.Ldate|
	_log.Ltime|
	_log.Lmicroseconds|
	_log.LUTC|
	_log.Lmsgprefix)

const headerLen = 4
const addrLen = 6
const broadcastIP = "255.255.255.255"

var ping = []byte("ping")
var pong = []byte("pong")

var bridgeHeader = []byte{85, 91, 20, 24}
var controlHeader = []byte{66, 99, 44, 22}
var dataHeader = []byte{11, 33, 33, 77}

func main() {
	upgrader := websocket.Upgrader{
		HandshakeTimeout: time.Second * 10,
		ReadBufferSize:   65536,
		WriteBufferSize:  65536,
		Subprotocols:     []string{"binary"},
		CheckOrigin: func(r *http.Request) bool {
			return true
		},
		EnableCompression: false,
	}

	mu := new(sync.Mutex)
	outgoingMessagesByAddr := make(map[Addr]chan []byte)

	handleWS := func(w http.ResponseWriter, r *http.Request) {
		identifier := fmt.Sprintf("%s - %s - %s", r.RemoteAddr, r.Method, r.URL)

		log := _log.New(os.Stdout, "", _log.Ldate|
			_log.Ltime|
			_log.Lmicroseconds|
			_log.LUTC|
			_log.Lmsgprefix)

		log.SetPrefix(fmt.Sprintf("%s - ", identifier))

		c, err := upgrader.Upgrade(w, r, nil)
		if err != nil {
			log.Printf("%s - warning: upgrade failed; err: %s", identifier, err)
			return
		}

		defer func() {
			_ = c.Close()
		}()

		log.Printf("upgraded")

		outgoingMessages := make(chan []byte, 1024)
		var src *Addr
		var dst *Addr

		var isBridge bool = false
		var isControl bool = false
		var header []byte
		var data []byte

		_ = isControl

		defer func() {
			if src != nil {
				mu.Lock()
				delete(outgoingMessagesByAddr, *src)
				addrs := slices.Collect(maps.Keys(outgoingMessagesByAddr))
				mu.Unlock()

				log.Printf("removed %#+v; now aware of %#+v", src, addrs)
			}
			close(outgoingMessages)
		}()

		//
		// ping / write loop in goroutine
		//

		go func() {
			t := time.NewTicker(time.Second * 1)

			var b []byte
			var err error

		loop:
			for {
				select {

				case <-t.C:
					err = c.WriteControl(websocket.PingMessage, ping, time.Now().Add(time.Second*2))
					if err != nil {
						log.Printf("error: ping failed; err: %s", err)
						_ = c.Close()
						break loop
					}

				case b = <-outgoingMessages:
					if b == nil {
						return
					}

					if len(b) == 4 && bytes.Equal(b, ping) {
						err = c.WriteControl(websocket.PingMessage, pong, time.Now().Add(time.Second*2))
						if err != nil {
							log.Printf("error: pong failed; err: %s", err)
							_ = c.Close()
							break loop
						}

						continue loop
					}

					err = c.WriteMessage(websocket.BinaryMessage, b)
					if err != nil {
						log.Printf("error: write failed; err: %s", err)
						_ = c.Close()
						break loop
					}
				}
			}
		}()
		runtime.Gosched()

		//
		// read loop
		//

		for {
			messageType, message, err := c.ReadMessage()

			wsErr, ok := err.(*websocket.CloseError)
			if ok && wsErr.Code == 1000 {
				log.Printf("warning: shutting down (%s)", err)
				break
			}

			netErr, ok := err.(net.Error)
			if ok && netErr.Timeout() {
				log.Printf("warning: read failed; err: %s", err)
				continue
			}

			if err != nil {
				log.Printf("error: read failed; err: %s", err)
				break
			}

			if messageType == websocket.PingMessage {
				select {
				case outgoingMessages <- ping:
				default:
				}

				continue
			}

			if messageType != websocket.BinaryMessage {
				log.Printf("warning: incorrect message type; wanted %d, got %d", websocket.BinaryMessage, messageType)
				continue
			}

			if len(message) < headerLen+addrLen+addrLen {
				log.Printf("warning: message too short at %d bytes; must be at least %d bytes", len(message), headerLen+addrLen+addrLen)
				continue
			}

			header = message[0:headerLen]
			if bytes.Equal(header, bridgeHeader) {
				isBridge = true
				isControl = false
			} else if bytes.Equal(header, controlHeader) {
				isBridge = false
				isControl = true
			} else if bytes.Equal(header, dataHeader) {
				isBridge = false
				isControl = false
			} else {
				log.Printf("warning: header %v not recognized; should be one of bridge: %v, control: %v or data: %v", header, bridgeHeader, controlHeader, dataHeader)
				continue
			}

			if src == nil {
				rawSrc := message[headerLen : headerLen+addrLen]

				src = &Addr{
					IP:   fmt.Sprintf("%d.%d.%d.%d", rawSrc[2], rawSrc[3], rawSrc[4], rawSrc[5]),
					Port: int(int(rawSrc[0])*256 + int(rawSrc[1])),
				}

				mu.Lock()
				outgoingMessagesByAddr[*src] = outgoingMessages
				addrs := slices.Collect(maps.Keys(outgoingMessagesByAddr))
				mu.Unlock()

				log.Printf("added %#+v; now aware of %#+v", src, addrs)
			}

			rawDst := message[headerLen+addrLen : headerLen+addrLen+addrLen]
			if dst == nil {
				dst = &Addr{}
			}
			dst.Port = int(int(rawDst[0])*256 + int(rawDst[1]))
			dst.IP = fmt.Sprintf("%d.%d.%d.%d", rawDst[2], rawDst[3], rawDst[4], rawDst[5])

			if len(message) > headerLen+addrLen+addrLen {
				data = message[headerLen+addrLen+addrLen:]
			} else {
				clear(data)
			}

			if isBridge {
				log.Printf("%d bytes - %#+v (%#+v)", len(data), data, string(data))

				// if we receive the hello announcement, we've got nothing more to do
				if bytes.Equal(data, []byte("g'day!")) {
					continue
				}

				//
				// all other bridge packets are considered to be for testing / debugging
				// and get echo'd back (with the addresses reversed)
				//

				respHeader := append([]byte{}, bridgeHeader...)
				respHeader = append(respHeader, message[headerLen+addrLen:headerLen+addrLen+addrLen]...)
				respHeader = append(respHeader, message[headerLen:headerLen+addrLen]...)

				select {
				case outgoingMessages <- append(respHeader, data...):
				default:
				}

				continue
			}

			if dst.IP == broadcastIP {
				var otherOutgoingMessages chan []byte

				mu.Lock()
				for otherAddr, possibleOtherOutgoingMessages := range outgoingMessagesByAddr {
					if otherAddr == *src {
						continue
					}

					if dst.Port != otherAddr.Port {
						continue
					}

					otherOutgoingMessages = possibleOtherOutgoingMessages

					break
				}
				mu.Unlock()

				if otherOutgoingMessages != nil {
					select {
					case otherOutgoingMessages <- message:
						// log.Printf("broadcasting %d bytes from %s to %s (as %s)", len(data), src.String(), otherAddr.String(), dst.String())
					default:
					}
				} else {
					log.Printf("warning: failed to broadcast %d bytes from %s to %s (as %s)", len(data), src.String(), "???", dst.String())
				}
			} else {
				mu.Lock()
				otherOutgoingMessages := outgoingMessagesByAddr[*dst]
				mu.Unlock()

				if otherOutgoingMessages != nil {
					select {
					case otherOutgoingMessages <- message:
						// log.Printf("unicasting %d bytes from %s to %s", len(data), src.String(), dst.String())
					default:
					}
				} else {
					log.Printf("warning: failed to unicast %d bytes from %s to %s", len(data), src.String(), dst.String())
				}
			}
		}
	}

	flag.Parse()
	http.HandleFunc("/ws", handleWS)
	http.HandleFunc("/ws/", handleWS)

	rawPort := os.Getenv("PORT")
	if rawPort == "" {
		rawPort = "8080"
	}

	server := &http.Server{
		Addr:    fmt.Sprintf("0.0.0.0:%s", rawPort),
		Handler: http.DefaultServeMux,
		ConnState: func(conn net.Conn, connState http.ConnState) {
			if connState == http.StateActive {
				_ = conn.(*net.TCPConn).SetReadBuffer(65536)
				_ = conn.(*net.TCPConn).SetWriteBuffer(65536)
			}
		},
	}

	log.Printf("listening on 0.0.0.0:%s...", rawPort)
	err := server.ListenAndServe()
	if err != nil {
		log.Fatal(err)
	}
}
