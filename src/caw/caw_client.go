package main

import (
    "context"
    "fmt"
    "time"

    "github.com/JanzenLiu/csci499_chengtsu/protos/caw"
    "github.com/JanzenLiu/csci499_chengtsu/protos/faz"
    "github.com/golang/protobuf/ptypes"
    "google.golang.org/grpc"
)

const (
    RegisterUser int32 = iota
    Follow
    Profile
    Caw
    Read
)

type CawClient struct {
    Funcs map[int32]string
    stub faz.FazServiceClient
}

func NewCawClient(cc grpc.ClientConnInterface) *CawClient {
    return &CawClient{
        Funcs: map[int32]string{
            RegisterUser: "RegisterUser",
            Follow: "Follow",
            Profile: "Profile",
            Caw: "Caw",
            Read: "Read",
        },
        stub: faz.NewFazServiceClient(cc),
    }
}

func (c *CawClient) HookAll() bool {
    success := true;
    for eventType, funcName := range c.Funcs {
        ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
        request := faz.HookRequest{
            EventType: eventType,
            EventFunction: funcName,
        }
        _, err := c.stub.Hook(ctx, &request)
        cancel()
        if err != nil {
            fmt.Printf("Failed to hooked the function %s. %v\n", funcName, err)
            success = false
        } else {
            fmt.Printf("Successfully hooked the function %s.\n", funcName)
        }
    }
    return success;
}

func (c *CawClient) UnhookAll() bool {
    success := true;
    for eventType, funcName := range c.Funcs {
        ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
        request := faz.UnhookRequest{
            EventType: eventType,
        }
        _, err := c.stub.Unhook(ctx, &request);
        cancel()
        if err != nil {
            fmt.Printf("Failed to unhook the function %s. %v\n", funcName, err)
            success = false
        } else {
            fmt.Printf("Successfully unhooked the function %s.\n", funcName)
        }
    }
    return success;
}

func (c *CawClient) RegisterUser(username string) bool {
    // Make the inner request packed in the generic request payload.
    innerRequest := caw.RegisteruserRequest{
        Username: username,
    }
    // Make the generic request.
    payload, err := ptypes.MarshalAny(&innerRequest)
    if err != nil {
        fmt.Println(err)
        return false
    }
    request := faz.EventRequest{
        EventType: RegisterUser,
        Payload: payload,
    }
    // Make RPC to the Faz service.
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()
    _, err = c.stub.Event(ctx, &request)
    if err != nil {
        fmt.Println(err)
        return false
    }
    fmt.Println("Successfully registered the user.")
    return true
}
