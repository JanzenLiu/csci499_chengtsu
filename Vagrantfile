# -*- mode: ruby -*-
# vi: set ft=ruby :

# All Vagrant configuration is done below. The "2" in Vagrant.configure
# configures the configuration version (we support older styles for
# backwards compatibility). Please don't change it unless you know what
# you're doing.
Vagrant.configure("2") do |config|
  # The most common configuration options are documented and commented below.
  # For a complete reference, please see the online documentation at
  # https://docs.vagrantup.com.

  # Every Vagrant development environment requires a box. You can search for
  # boxes at https://vagrantcloud.com/search.
  config.vm.box = "generic/ubuntu2004"

  # Disable automatic box update checking. If you disable this, then
  # boxes will only be checked for updates when the user runs
  # `vagrant box outdated`. This is not recommended.
  # config.vm.box_check_update = false

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine. In the example below,
  # accessing "localhost:8080" will access port 80 on the guest machine.
  # NOTE: This will enable public access to the opened port
  # config.vm.network "forwarded_port", guest: 80, host: 8080

  # Create a forwarded port mapping which allows access to a specific port
  # within the machine from a port on the host machine and only allow access
  # via 127.0.0.1 to disable public access
  # config.vm.network "forwarded_port", guest: 80, host: 8080, host_ip: "127.0.0.1"

  # Create a private network, which allows host-only access to the machine
  # using a specific IP.
  # config.vm.network "private_network", ip: "192.168.33.10"

  # Create a public network, which generally matched to bridged network.
  # Bridged networks make the machine appear as another physical device on
  # your network.
  # config.vm.network "public_network"

  # Share an additional folder to the guest VM. The first argument is
  # the path on the host to the actual folder. The second argument is
  # the path on the guest to mount the folder. And the optional third
  # argument is a set of non-required options.
  # config.vm.synced_folder ".", "/vagrant", disabled: true
  config.vm.synced_folder ".", "/home/vagrant/csci499_chengtsu", type: "rsync",
    rsync__exclude: [".git/", ".vagrant/", "build/"],
    create: true

  # Provider-specific configuration so you can fine-tune various
  # backing providers for Vagrant. These expose provider-specific options.
  # Example for VirtualBox:
  #
  # config.vm.provider "virtualbox" do |vb|
  #   # Display the VirtualBox GUI when booting the machine
  #   vb.gui = true
  #
  #   # Customize the amount of memory on the VM:
  #   vb.memory = "1024"
  # end
  #
  # View the documentation for the provider you are using for more
  # information on available options.

  # Enable provisioning with a shell script. Additional provisioners such as
  # Ansible, Chef, Docker, Puppet and Salt are also available. Please see the
  # documentation for more information about their specific syntax and use.
  config.vm.provision "shell", inline: <<-SHELL
    # Install essentials.
    sudo apt-get update
    sudo apt install -y build-essential autoconf libtool pkg-config
    sudo apt install -y clang
    sudo apt install -y cmake
    # Install glog
    git clone https://github.com/google/glog.git
    pushd glog/
    cmake -H. -B build -G "Unix Makefiles"
    cmake --build build
    sudo cmake --build build --target install
    popd
    # Install gflags
    wget https://github.com/gflags/gflags/archive/v2.2.2.tar.gz
    tar xzf v2.2.2.tar.gz
    pushd gflags-2.2.2/
    mkdir build && cd build
    cmake -DBUILD_SHARED_LIBS=ON -DBUILD_STATIC_LIBS=ON -DBUILD_gflags_LIB=ON ..
    make
    sudo make install
    popd
    # Install googletest
    git clone https://github.com/google/googletest.git -b release-1.10.0
    pushd googletest
    mkdir build
    cd build
    cmake -DBUILD_SHARED_LIBS=ON ..
    make
    sudo make install
    popd
    # Install gRPC and Protobuf
    git clone --recurse-submodules -b v1.35.0 https://github.com/grpc/grpc
    pushd grpc
    mkdir -p cmake/build
    cd cmake/build
    cmake -DgRPC_INSTALL=ON -DgRPC_BUILD_TESTS=OFF -DCMAKE_INSTALL_PREFIX=/usr/local ../..
    make -j4
    sudo make install
    popd
    sudo apt install -y libgtest-dev
    # Install Go
    wget https://golang.org/dl/go1.16.2.linux-amd64.tar.gz
    sudo tar -C /usr/local -xzf go1.16.2.linux-amd64.tar.gz
    export PATH=$PATH:/usr/local/go/bin
    # Install Go plugins for the protocol compiler
    export GO111MODULE=on  # Enable module mode
    export MY_HOME=/home/vagrant
    GOPATH=$MY_HOME/go go get google.golang.org/protobuf/cmd/protoc-gen-go google.golang.org/grpc/cmd/protoc-gen-go-grpc
    sudo chmod a+w -R $MY_HOME/go
    sudo chown vagrant -R $MY_HOME/go
    # Set environment variables
    echo $'\n' >> $MY_HOME/.profile
    echo 'export PATH=$PATH:/usr/local/go/bin' >> $MY_HOME/.profile
    echo 'export PATH=$PATH:$MY_HOME/go/bin' >> $MY_HOME/.profile
    echo 'export GO111MODULE=on' >> $MY_HOME/.profile
  SHELL
end
