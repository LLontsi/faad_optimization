package router

import (
	"fmt"
	"net"
	"regexp"
	"syscall"
	"time"
)

var pathRegex = regexp.MustCompile(`^(GET|POST|PUT|DELETE|PATCH|HEAD|OPTIONS)\s+/function/([a-zA-Z0-9_-]+)`)

// PeekFunction lit JUSTE la première ligne pour extraire la fonction
// Utilise MSG_PEEK pour ne PAS consommer les données du socket
func PeekFunction(conn net.Conn) (string, error) {
	conn.SetReadDeadline(time.Now().Add(2 * time.Second))
	defer conn.SetReadDeadline(time.Time{})

	tcpConn := conn.(*net.TCPConn)
	
	// Obtenir le FD
	var fd int
	rawConn, err := tcpConn.SyscallConn()
	if err != nil {
		return "", err
	}
	
	err = rawConn.Control(func(fdArg uintptr) {
		fd = int(fdArg)
	})
	if err != nil {
		return "", err
	}

	// Peek 512 bytes sans consommer
	buf := make([]byte, 512)
	n, _, err := syscall.Recvfrom(fd, buf, syscall.MSG_PEEK)
	if err != nil {
		return "", fmt.Errorf("peek failed: %w", err)
	}

	// Extraire la première ligne
	firstLine := string(buf[:n])
	
	matches := pathRegex.FindStringSubmatch(firstLine)
	if len(matches) < 3 {
		return "", fmt.Errorf("invalid request line")
	}

	return matches[2], nil
}
