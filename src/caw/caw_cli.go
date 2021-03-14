package main

import (
    "flag"
    "fmt"

    "github.com/JanzenLiu/csci499_chengtsu/protos/caw"
    "google.golang.org/grpc"
)

// Define command-line flags.
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

func printProfileReply(profileReply *caw.ProfileReply) {
    following := profileReply.GetFollowing()
    followers := profileReply.GetFollowers()
    fmt.Println("{");
    fmt.Printf( "  following (size=%d): %v\n", len(following), following)
    fmt.Printf( "  followers (size=%d): %v\n", len(followers), followers)
    fmt.Println("}");
}

func main() {
    flag.Parse()
    var opts []grpc.DialOption
    opts = append(opts, grpc.WithInsecure())
    opts = append(opts, grpc.WithBlock())
    conn, err := grpc.Dial(*fazAddr, opts...)
    if err != nil {
        fmt.Printf("Failed to dial. %v", err)
    }
    defer conn.Close()
    client := NewCawClient(conn)

    // Handle flag -hook_all.
    if *hookAll {
        fmt.Println("Hooking all Caw functions to the Faz layer...")
        client.HookAll()
    }

    // Handle flag -registeruser.
    if *registerUser != "" {
        client.RegisterUser(*registerUser);
    }
    // Handle flag -follow.
    if *follow != "" {
        if *user == "" {
            fmt.Println("You need to login to follow a user.")
        } else {
            client.Follow(*user, *follow);
        }
    }
    // Handle flag -profile.
    if *profile {
        if *user == "" {
            fmt.Println("You need to login to get the user's profile.")
        } else {
            profileReply := client.Profile(*user);
            if profileReply != nil {
                printProfileReply(profileReply)
            }
        }
    }

    // Handle flag -unhook_all.
    if *unhookAll {
        fmt.Println("Unhooking all Caw functions from the Faz layer...")
        client.UnhookAll();
    }
}
