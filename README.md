# csci499_chengtsu

<div>

[![Status](https://img.shields.io/badge/status-active-success.svg)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](/LICENSE)

</div>

An FaaS platform and a social network system on top of it. For USC CSCI499.

## Architecture
![Architecture and Workflow](./images/arch_and_workflow.svg)

## Pre-reqs
To build and run this app locally you will need a few things:
- Install **googletest**
- Install **gflags**
- Install **glog**
- Install **gRPC** and **Protobuf**

## Build
```bash
mkdir build && cd build
cmake ..
make
```

## Usage
To get the Caw platform work, you need to first start the KVStore server, 
and then start the FaaS server. Then you can use the Caw command-line tool
to use the Caw functionalities. 

Assume you are already in a directory containing the built executables.
Below are instructions to get the system work.

To run the KVStore server
```bash
./kvstore_server
```

To run the FaaS server
```bash
./faz_server
```

To use the Caw command-line tool for different purposes
- To hook all Caw functions: 
`./caw_cli --hook_all`

- To register a user: 
`./caw_cli --registeruser <username>`

- To follow another user on behalf of a user: 
`./caw_cli --user <username> --follow <to_follow>`

- To get a user's profile: 
`./caw_cli --user <username> --profile`

- To post a caw on behalf of a user: 
`./caw_cli --user <username> --caw <text>`

- To post a caw replying an existing caw on behalf of a user: 
`./caw_cli --user <username> --caw <text> --reply <parent_caw_id>`

- To read a caw thread starting from a caw: 
`./caw_cli --read <caw_id>`

- To unhook all Caw functions: 
`./caw_cli --unhook_all`

> **Note!** The command-line tool also supports chaining flags to enable you 
> to run multiple functions in one command for convenience. For example you
> can do some thing like
> ```bash
> ./caw_cli --hook_all --registeruser Reiner --user Reiner --follow Zeke \ 
>   --caw "I am the Armored Titan, he is the Colossal Titan" \
>   --profile --unhook_all
>```
