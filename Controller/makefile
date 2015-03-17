all: controller

controller: Controller.cpp Controller.h ProtocolMessage.cpp ProtocolMessage.h
	g++ ProtocolMessage.cpp Controller.cpp -o Controller -lpthread

clean:
	rm Controller
