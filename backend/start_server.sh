pkill -9 -f PaperCrawlerServerHotPlug 2>/dev/null

cd build
LD_LIBRARY_PATH=./modules:$LD_LIBRARY_PATH ./PaperCrawlerServerHotPlug