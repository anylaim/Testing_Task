CXX = g++
CXXFLAGS = -Wall -Wextra -O2 -std=c++17
TARGET = Testing_Task
SERVICE_NAME = Testing_Task.service

SRC = main.cpp EpollServer.cpp ThreadPool.cpp Client.cpp Utils.cpp
OBJ = $(SRC:.cpp=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJ)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	rm -f $(OBJ) $(TARGET)

install: $(TARGET)
	mkdir -p /usr/local/bin/
	cp $(TARGET) /usr/local/bin/$(TARGET)

install-service: install
	cp packaging/$(SERVICE_NAME) /etc/systemd/system/
	systemctl daemon-reload

enable-service: install-service
	systemctl enable $(SERVICE_NAME)

start-service: enable-service
	systemctl start $(SERVICE_NAME)

status-service:
	systemctl status $(SERVICE_NAME)

stop-service:
	systemctl stop $(SERVICE_NAME)

restart-service: stop-service start-service

uninstall:
	rm -f /usr/local/bin/$(TARGET)
	systemctl stop $(SERVICE_NAME) || true
	systemctl disable $(SERVICE_NAME) || true
	rm -f /etc/systemd/system/$(SERVICE_NAME)
	systemctl daemon-reload

.PHONY: clean install install-service enable-service start-service status-service stop-service restart-service uninstall