# csci499_chengtsu

<div>

[![Status](https://img.shields.io/badge/status-active-success.svg)]()
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](/LICENSE)

</div>

An FaaS platform and a social network system on top of it. For USC CSCI499 - Robust Systems Design and Implementation.

## Table of Contents

- [Architecture](#arch)
- [Pre-reqs](#pre-reqs)
- [Build](#build)
- [Usage](#usage)
- [Test](#test)
- [Authors](#authors)
- [Acknowledgments](#acks)

## Architecture <a name = "arch"></a>
![Architecture and Workflow](./images/arch_and_workflow.svg)

As shown in the diagram, there will be three executables built in this project (except tests), they are:

- **caw_cli**: The Caw command-line tool who accepts the user's input, sends requests to 
the Faz Server through the `CawClient` accordingly, and displays response messages to the 
user based on the return from the Faz Server.

- **faz_server**: The Faz Server who hosts a `FazService`, which executes the corresponding
Caw handler functions based on the incoming event types, and sends back response messages. 
During the execution of Caw handler functions, interactions with a KVStore Server may occur,
through a `KVStoreClient`.

- **kvstore_server**: The KVStore Server who hosts a `KVStoreService`, which performs
put/get/remove operations to the underlying `KVStore` instance as requested, and sends back
response messages to remote callers.

## Pre-reqs <a name = "pre-reqs"></a>
If you are using vagrant, the `Vagrantfile` at the project root directory can install all 
dependencies for you automatically. Assume you have cloned the project on your host machine
and are already in the project root directory, you just need to to create your guest machine 
and connect to it.
```
vagrant up
vagrant ssh
cd csci499_chengtsu
```

If not using vagrant, to build and run this project locally you will need a few things:
- Install **googletest**
- Install **gflags**
- Install **glog**
- Install **gRPC** and **Protobuf**

## Build <a name = "build"></a>
Assume you are already in the project root directory (on your vagrant guest, or on your
host machine after all pre-requisites installed), run:
```
mkdir build && cd build
cmake ..
make
```

## Usage <a name = "usage"></a>
To get the Caw platform work, you need to first start the KVStore server, 
and then start the FaaS server. Then you can use the Caw command-line tool
to use the Caw functionalities. 

Assume you are already in a directory containing the built executables.
Below are instructions to get the system work.

To run the KVStore server
```
./kvstore_server
```

To run the FaaS server
```
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

For Example:
```
./caw_cli --hook_all
./caw_cli --registeruser Eren
./caw_cli --registeruser Mikasa
./caw_cli --user Mikasa --follow Eren
./caw_cli --user Mikasa --profile
./caw_cli --user Eren --caw "Sit down, Reiner"
./caw_cli --unhook_all
```

> **Note!** The command-line tool also supports chaining flags to enable you 
> to run multiple functions in one command for convenience. For example you
> can do some thing like
> ```bash
> ./caw_cli --hook_all --registeruser Reiner --user Reiner --follow Zeke \ 
>   --caw "I am the Armored Titan, he is the Colossal Titan" \
>   --profile --unhook_all
>```

## Test <a name = "test"></a>
Assume you are already in a directory containing the built executables.
Below are instructions to run the tests.

To run the KVStore (storage only) test
```
./kvstore_test
```

To run the Caw handler function test
```
./caw_handler_test
```

To run the KVStore shell to do interactive testing.
Note that you can even run this when the other executables are running to 
inspect the content of the KVStore, or even update the content to create
some extreme cases (for example, a posted caw accidentally lost in the database).
```
./kvstore_shell_test
```

## Authors <a name = "authors"></a>
- [Cheng-Tsung Liu](https://github.com/JanzenLiu)

## Acknowledgements <a name = "acks"></a>
- [Adam Egyed](https://github.com/adamegyed)
- [Barath Raghavan](https://raghavan.usc.edu/)
