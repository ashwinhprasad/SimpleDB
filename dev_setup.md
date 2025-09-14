# Docker setup
```
docker run --name dabbadb_cpp -v $(pwd):/app --network=host -it ubuntu bash

apt update -y && apt install build-essential -y && apt install cmake

cd /app && mkdir build && cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ..

apt install python3 -y && apt install python3-venv -y

python3 -m venv venv && source venv/bin/activate && pip install conan
```


# Conan Setup

Once the conanfile is written, run the following commands to set up conan and install dependencies.

```bash
conan profile detect --force # Automatically detects the default profile


conan install . --output-folder=build --build=missing # To install the dependencies in the conanfile.txt

cmake -B build -S . -DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake

cd build && cmake --build .
```


