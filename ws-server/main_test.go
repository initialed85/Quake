package main

import (
	"bytes"
	"sync"
	"testing"
)

func TestBroadcastMessageFansOutByPort(t *testing.T) {
	mu := new(sync.Mutex)
	message := []byte{1, 2, 3}
	src := Addr{IP: "10.0.0.1", Port: 26000}
	matchingOne := make(chan []byte, 1)
	matchingTwo := make(chan []byte, 1)
	nonMatching := make(chan []byte, 1)
	sender := make(chan []byte, 1)

	handled := broadcastMessage(mu, map[Addr]chan []byte{
		src:                           sender,
		{IP: "10.0.0.2", Port: 26000}: matchingOne,
		{IP: "10.0.0.3", Port: 26000}: matchingTwo,
		{IP: "10.0.0.4", Port: 26001}: nonMatching,
	}, src, Addr{IP: broadcastIP, Port: 26000}, message)

	if !handled {
		t.Fatal("broadcast was not handled")
	}

	for name, ch := range map[string]chan []byte{
		"matching endpoint 1": matchingOne,
		"matching endpoint 2": matchingTwo,
	} {
		select {
		case got := <-ch:
			if !bytes.Equal(got, message) {
				t.Errorf("%s received %v, want %v", name, got, message)
			}
		default:
			t.Errorf("%s did not receive the broadcast", name)
		}
	}

	for name, ch := range map[string]chan []byte{
		"sender":            sender,
		"non-matching port": nonMatching,
	} {
		select {
		case got := <-ch:
			t.Errorf("%s received unexpected message %v", name, got)
		default:
		}
	}
}

func TestBroadcastMessageReportsNoRecipient(t *testing.T) {
	mu := new(sync.Mutex)
	handled := broadcastMessage(
		mu,
		map[Addr]chan []byte{{IP: "10.0.0.2", Port: 26001}: make(chan []byte, 1)},
		Addr{IP: "10.0.0.1", Port: 26000},
		Addr{IP: broadcastIP, Port: 26000},
		[]byte{1},
	)

	if handled {
		t.Fatal("broadcast reported a recipient on a different port")
	}
}
