
### Installtion on Ubuntu

```

# Installing Build Dependencies
sudo apt-get install build-essential libglib2.0-dev libglib2.0-0

# To compile and build the vega server
make

# To compile and generate the shared object files for the example controllers
make prepare_examples

# Set LD_LIBRARY_PATH
export LD_LIBRARY_PATH=/usr/local/lib/

# To Start the Server (the default port is 5000)
./vega


```
