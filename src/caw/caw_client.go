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

// Caw event types to register with the corresponding functions.
const (
    RegisterUser int32 = iota
    Follow
    Profile
    Caw
    Read
)

// A client to make RPC to the remote Faz gRPC service on
// behalf of the Caw platform.
type CawClient struct {
    // Table that maps a Caw event type to the predefined function
    // name known by the Faz service.
    Funcs map[int32]string
    // Stub to make the actual RPC.
    stub faz.FazServiceClient
}

// Creates a `CawClient` struct who uses the given client connection
// to communicate with the Faz service.
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

// Hooks all Caw functions on Faz, and returns true on success.
func (c *CawClient) HookAll() bool {
    success := true;
    for eventType, funcName := range c.Funcs {
        // Make the request.
        request := faz.HookRequest{
            EventType: eventType,
            EventFunction: funcName,
        }
        // Make RPC to the Faz service.
        ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
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

// Unhooks all Caw functions from Faz, and returns true on success.
func (c *CawClient) UnhookAll() bool {
    success := true;
    for eventType, funcName := range c.Funcs {
        // Make the request.
        request := faz.UnhookRequest{
            EventType: eventType,
        }
        // Make RPC to the Faz service.
        ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
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

// Sends an `RegisterUser` event to Faz and returns true on success.
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

// Sends an `Follow` event to Faz and returns true on success.
func (c *CawClient) Follow(username, toFollow string) bool {
    // Make the inner request packed in the generic request payload.
    innerRequest := caw.FollowRequest{
        Username: username,
        ToFollow: toFollow,
    }
    // Make the generic request.
    payload, err := ptypes.MarshalAny(&innerRequest)
    if err != nil {
        fmt.Println(err)
        return false
    }
    request := faz.EventRequest{
        EventType: Follow,
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
    fmt.Println("Successfully followed the user.")
    return true
}

// Sends an `Profile` event to Faz and returns a `ProfileReply`
// message containing the desired information on success.
func (c *CawClient) Profile(username string) *caw.ProfileReply {
    // Make the inner request packed in the generic request payload.
    innerRequest := caw.ProfileRequest{
        Username: username,
    }
    // Make the generic request.
    payload, err := ptypes.MarshalAny(&innerRequest)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    request := faz.EventRequest{
        EventType: Profile,
        Payload: payload,
    }
    // Make RPC to the Faz service.
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()
    response, err := c.stub.Event(ctx, &request)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    // Get the ProfileReply message.
    payload = response.GetPayload()
    if payload == nil {
        return nil
    }
    innerResponse := new(caw.ProfileReply)
    err = ptypes.UnmarshalAny(payload, innerResponse)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    return innerResponse
}

// Sends an `Caw` event to Faz and returns the Caw message posted on success.
func (c *CawClient) Caw(username, text, parentId string) *caw.Caw {
    // Make the inner request packed in the generic request payload.
    innerRequest := caw.CawRequest{
        Username: username,
        Text: text,
        ParentId: []byte(parentId),
    }
    // Make the generic request.
    payload, err := ptypes.MarshalAny(&innerRequest)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    request := faz.EventRequest{
        EventType: Caw,
        Payload: payload,
    }
    // Make RPC to the Faz service.
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()
    response, err := c.stub.Event(ctx, &request)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    // Get the Caw message.
    payload = response.GetPayload()
    if payload == nil {
        return nil
    }
    innerResponse := new(caw.CawReply)
    err = ptypes.UnmarshalAny(payload, innerResponse)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    return innerResponse.GetCaw()
}

func (c *CawClient) Read(cawId string) []*caw.Caw {
    // Make the inner request packed in the generic request payload.
    innerRequest := caw.ReadRequest{
        CawId: []byte(cawId),
    }
    // Make the generic request.
    payload, err := ptypes.MarshalAny(&innerRequest)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    request := faz.EventRequest{
        EventType: Read,
        Payload: payload,
    }
    // Make RPC to the Faz service.
    ctx, cancel := context.WithTimeout(context.Background(), 10*time.Second)
    defer cancel()
    response, err := c.stub.Event(ctx, &request)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    // Get the Caw message.
    payload = response.GetPayload()
    if payload == nil {
        return nil
    }
    innerResponse := new(caw.ReadReply)
    err = ptypes.UnmarshalAny(payload, innerResponse)
    if err != nil {
        fmt.Println(err)
        return nil
    }
    return innerResponse.GetCaws()
}
