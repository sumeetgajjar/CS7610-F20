set -e
pwd
git clone https://github.com/google/glog.git
cd glog
cmake -H. -Bbuild -G "Unix Makefiles"
cmake --build build
cmake --build build --target test
cmake --build build --target install
