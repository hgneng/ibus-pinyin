sudo apt install -y libpyzy-dev
sudo apt install -y libsqlite3-dev

./autogen.sh
./configure  --prefix=/usr --libexecdir=/usr/lib/ibus
make
sudo make install
