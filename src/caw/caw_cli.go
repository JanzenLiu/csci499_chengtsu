package main

import (
    "flag"
    "fmt"

    _ "github.com/JanzenLiu/csci499_chengtsu/protos/caw"
    "google.golang.org/grpc"
)

var (
	fazAddr      = flag.String("faz_addr", "localhost:50000", "The server Faz address in the format of host:port")
	registerUser = flag.String("registeruser", "", "Registers the given username")
	user         = flag.String("user", "", "Logs in as the given username")
	text         = flag.String("caw", "", "Creates a new caw with the given text")
    reply        = flag.String("reply", "", "Indicates that the new caw is a reply to the given id")
    follow       = flag.String("follow", "", "Starts following the given username")
    read         = flag.String("read", "", "Reads the caw thread starting at the given id")
    profile      = flag.Bool("profile", false, "Gets the user’s profile of following and followers")
    hookAll      = flag.Bool("hook_all", false, "Hooks all Caw functions to the Faz layer.")
    unhookAll    = flag.Bool("unhook_all", false, "Unhooks all Caw functions from the Faz layer.")
)

func main() {
	flag.Parse()
	var opts []grpc.DialOption
	opts = append(opts, grpc.WithInsecure())
	opts = append(opts, grpc.WithBlock())
	conn, err := grpc.Dial(*fazAddr, opts...)
	if err != nil {
		fmt.Printf("fail to dial: %v", err)
	}
	defer conn.Close()
	client := NewCawClient(conn)

	// Handle flag -hook_all.
	if *hookAll {
        fmt.Println("Hooking all Caw functions to the Faz layer...")
        client.HookAll();
    }

    // Handle flag --registeruser.
    if *registerUser != "" {
        client.RegisterUser(*registerUser);
    }

    // Handle flag -unhook_all.
    if *unhookAll {
        fmt.Println("Unhooking all Caw functions from the Faz layer...")
        client.UnhookAll();
    }
}
