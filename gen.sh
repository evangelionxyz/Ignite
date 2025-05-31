#!/usr/bin/env bash

# change ownership of the home directory
# to the current user
sudo chown -R $USER: $HOME

# install dependenceis
sudo apt update
sudo apt install libwayland-dev libxkbcommon-dev xorg-dev zlib1g-dev libfmt-dev zenity gdb libgmock-dev ninja-build -y

# mono installation
sudo apt install ca-certificates gnupg
sudo gpg --homedir /tmp --no-default-keyring --keyring /usr/share/keyrings/mono-official-archive-keyring.gpg --keyserver hkp://keyserver.ubuntu.com:80 --recv-keys 3FA7E0328081BFF6A14DA29AA6A19B38D3D831EF
echo "deb [signed-by=/usr/share/keyrings/mono-official-archive-keyring.gpg] https://download.mono-project.com/repo/ubuntu stable-focal main" | sudo tee /etc/apt/sources.list.d/mono-official-stable.list
sudo apt update
sudo apt install build-essential mono-complete mono-devel mono-dbg libicu-dev -y
sudo snap install dotnet-sdk --classic

# Vulkan installation
wget -qO- https://packages.lunarg.com/lunarg-signing-key-pub.asc | sudo tee /etc/apt/trusted.gpg.d/lunarg.asc
sudo wget -qO /etc/apt/sources.list.d/lunarg-vulkan-noble.list http://packages.lunarg.com/vulkan/lunarg-vulkan-noble.list
sudo apt update
sudo apt install vulkan-sdk
sudo apt install libvulkan1 mesa-vulkan-drivers vulkan-utils -y

# run the python setup script
pushd ./scripts
sudo python3 setup.py
popd