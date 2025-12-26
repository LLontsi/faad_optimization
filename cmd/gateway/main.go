package main

import (
	"fmt"
	"log"
	"net"
	"os"

	"sendfd-simple/pkg/fdtransfer"
	"sendfd-simple/pkg/router"
)

var functions = map[string]string{
	"echo":   "/var/run/functions/echo.sock",
	"resize": "/var/run/functions/resize.sock",
}

func main() {
	log.SetPrefix("[Gateway] ")
	log.Println("Starting sendfd Gateway...")

	listener, err := net.Listen("tcp", ":9000")
	if err != nil {
		log.Fatalf("Failed to listen: %v", err)
	}
	defer listener.Close()

	log.Println("Gateway listening on :9000")

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Printf("Accept error: %v", err)
			continue
		}
		go handleConnection(conn)
	}
}

func handleConnection(conn net.Conn) {
	defer func() {
		// Ne fermer que si on n'a PAS transféré le FD
	}()

	clientAddr := conn.RemoteAddr().String()
	log.Printf("Connection from %s", clientAddr)

	// Peek la fonction SANS consommer les données
	functionName, err := router.PeekFunction(conn)
	if err != nil {
		log.Printf("Error peeking function: %v", err)
		sendError(conn, 400, "Bad Request")
		conn.Close()
		return
	}

	log.Printf("Target function: %s", functionName)

	// Vérifier que la fonction existe
	socketPath, exists := functions[functionName]
	if !exists {
		log.Printf("Function %s not found", functionName)
		sendError(conn, 404, "Function Not Found")
		conn.Close()
		return
	}

	// Vérifier que le socket existe
	if _, err := os.Stat(socketPath); err != nil {
		log.Printf("Socket %s not accessible", socketPath)
		sendError(conn, 503, "Function Unavailable")
		conn.Close()
		return
	}

	// Connecter au conteneur
	unixConn, err := net.Dial("unix", socketPath)
	if err != nil {
		log.Printf("Error connecting to %s: %v", socketPath, err)
		sendError(conn, 503, "Function Unavailable")
		conn.Close()
		return
	}
	defer unixConn.Close()

	// Transférer le FD
	tcpConn := conn.(*net.TCPConn)
	unixConnTyped := unixConn.(*net.UnixConn)

	err = fdtransfer.SendFD(unixConnTyped, tcpConn, functionName)
	if err != nil {
		log.Printf("Error transferring FD: %v", err)
		sendError(conn, 500, "Internal Server Error")
		conn.Close()
		return
	}

	log.Printf("FD transferred to %s", functionName)
	
	// NE PAS fermer conn - le conteneur l'a maintenant
}

func sendError(conn net.Conn, code int, message string) {
	response := fmt.Sprintf("HTTP/1.1 %d %s\r\n"+
		"Content-Type: text/plain\r\n"+
		"Content-Length: %d\r\n"+
		"Connection: close\r\n"+
		"\r\n"+
		"%s", code, message, len(message), message)
	conn.Write([]byte(response))
}
