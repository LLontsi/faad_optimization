package fdtransfer

import (
	"fmt"
	"net"
	"syscall"
)

// SendFD envoie juste le FD + le nom de la fonction (pas de header!)
func SendFD(unixConn *net.UnixConn, tcpConn *net.TCPConn, functionName string) error {
	// Dupliquer le FD
	var originalFd int
	rawConn, err := tcpConn.SyscallConn()
	if err != nil {
		return fmt.Errorf("failed to get syscall conn: %w", err)
	}
	
	err = rawConn.Control(func(fdArg uintptr) {
		originalFd = int(fdArg)
	})
	if err != nil {
		return fmt.Errorf("failed to get raw fd: %w", err)
	}
	
	fd, err := syscall.Dup(originalFd)
	if err != nil {
		return fmt.Errorf("failed to duplicate fd: %w", err)
	}

	// Envoyer le nom de fonction comme message (1 byte de taille + nom)
	message := []byte(functionName)
	msgLen := byte(len(message))
	fullMsg := append([]byte{msgLen}, message...)

	rights := syscall.UnixRights(fd)

	unixFile, err := unixConn.File()
	if err != nil {
		syscall.Close(fd)
		return fmt.Errorf("failed to get unix file: %w", err)
	}
	defer unixFile.Close()

	unixFd := int(unixFile.Fd())

	err = syscall.Sendmsg(unixFd, fullMsg, rights, nil, 0)
	if err != nil {
		syscall.Close(fd)
		return fmt.Errorf("sendmsg failed: %w", err)
	}

	return nil
}

// ReceiveFD reçoit le FD + nom de fonction
func ReceiveFD(unixConn *net.UnixConn) (int, string, error) {
	buf := make([]byte, 256)
	oob := make([]byte, syscall.CmsgSpace(4))

	unixFile, err := unixConn.File()
	if err != nil {
		return -1, "", fmt.Errorf("failed to get unix file: %w", err)
	}
	defer unixFile.Close()

	unixFd := int(unixFile.Fd())

	n, oobn, _, _, err := syscall.Recvmsg(unixFd, buf, oob, 0)
	if err != nil {
		return -1, "", fmt.Errorf("recvmsg failed: %w", err)
	}

	if n < 1 {
		return -1, "", fmt.Errorf("message too short")
	}

	msgLen := int(buf[0])
	if msgLen > n-1 {
		return -1, "", fmt.Errorf("invalid message length")
	}

	functionName := string(buf[1 : 1+msgLen])

	if oobn == 0 {
		return -1, "", fmt.Errorf("no control message received")
	}

	msgs, err := syscall.ParseSocketControlMessage(oob[:oobn])
	if err != nil {
		return -1, "", fmt.Errorf("failed to parse control message: %w", err)
	}

	if len(msgs) == 0 {
		return -1, "", fmt.Errorf("no control messages found")
	}

	fds, err := syscall.ParseUnixRights(&msgs[0])
	if err != nil {
		return -1, "", fmt.Errorf("failed to parse unix rights: %w", err)
	}

	if len(fds) == 0 {
		return -1, "", fmt.Errorf("no file descriptor received")
	}

	fd := fds[0]
	return fd, functionName, nil
}
