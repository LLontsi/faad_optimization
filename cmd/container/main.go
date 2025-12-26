package main

import (
	"bufio"
	"fmt"
	"io"
	"log"
	"net"
	"net/http"
	"os"

	"sendfd-simple/pkg/fdtransfer"
)

func main() {
	log.SetPrefix("[Container] ")
	log.Println("Starting container...")

	socketPath := os.Getenv("FUNCTION_SOCKET")
	if socketPath == "" {
		socketPath = "/function.sock"
	}

	os.Remove(socketPath)

	listener, err := net.Listen("unix", socketPath)
	if err != nil {
		log.Fatalf("Failed to listen: %v", err)
	}
	defer listener.Close()

	log.Printf("Listening on %s", socketPath)

	for {
		conn, err := listener.Accept()
		if err != nil {
			log.Printf("Accept error: %v", err)
			continue
		}
		go handleFD(conn)
	}
}

func handleFD(unixConn net.Conn) {
	defer unixConn.Close()

	unixConnTyped := unixConn.(*net.UnixConn)
	
	// Recevoir le FD
	fd, functionName, err := fdtransfer.ReceiveFD(unixConnTyped)
	if err != nil {
		log.Printf("Error receiving FD: %v", err)
		return
	}

	log.Printf("Received FD %d for function %s", fd, functionName)

	// Créer une connexion depuis le FD
	file := os.NewFile(uintptr(fd), "tcp")
	if file == nil {
		log.Printf("Error creating file from FD")
		return
	}
	defer file.Close()

	tcpConn, err := net.FileConn(file)
	if err != nil {
		log.Printf("Error creating conn: %v", err)
		return
	}
	defer tcpConn.Close()

	// Lire la requête HTTP COMPLÈTE depuis le début
	reader := bufio.NewReader(tcpConn)
	req, err := http.ReadRequest(reader)
	if err != nil {
		log.Printf("Error reading request: %v", err)
		sendError(tcpConn, 400, "Bad Request")
		return
	}

	log.Printf("Processing %s %s", req.Method, req.URL.Path)

	// Traiter la requête
	w := &responseWriter{conn: tcpConn}
	
	handleEcho(w, req)
	
	log.Println("Request completed")
}

type responseWriter struct {
	conn        net.Conn
	wroteHeader bool
	statusCode  int
	headers     http.Header
}

func (w *responseWriter) Header() http.Header {
	if w.headers == nil {
		w.headers = make(http.Header)
	}
	return w.headers
}

func (w *responseWriter) Write(data []byte) (int, error) {
	if !w.wroteHeader {
		w.WriteHeader(http.StatusOK)
	}
	return w.conn.Write(data)
}

func (w *responseWriter) WriteHeader(statusCode int) {
	if w.wroteHeader {
		return
	}
	w.wroteHeader = true
	w.statusCode = statusCode

	fmt.Fprintf(w.conn, "HTTP/1.1 %d %s\r\n", statusCode, http.StatusText(statusCode))
	
	for key, values := range w.headers {
		for _, value := range values {
			fmt.Fprintf(w.conn, "%s: %s\r\n", key, value)
		}
	}
	
	fmt.Fprintf(w.conn, "\r\n")
}

func handleEcho(w http.ResponseWriter, r *http.Request) {
	body, _ := io.ReadAll(r.Body)
	
	response := fmt.Sprintf("=== Echo Function ===\n\n"+
		"Method: %s\n"+
		"Path: %s\n"+
		"Body: %s\n", r.Method, r.URL.Path, string(body))
	
	w.Header().Set("Content-Type", "text/plain")
	w.WriteHeader(http.StatusOK)
	w.Write([]byte(response))
}

func sendError(conn net.Conn, code int, message string) {
	response := fmt.Sprintf("HTTP/1.1 %d %s\r\n"+
		"Content-Type: text/plain\r\n"+
		"Content-Length: %d\r\n"+
		"\r\n"+
		"%s", code, message, len(message), message)
	conn.Write([]byte(response))
}
