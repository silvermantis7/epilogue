epilogue: src/gui.cpp src/gui.hpp src/process_messages.hpp src/server.hpp
	${CXX} src/gui.cpp -Ofast -o epilogue \
		$$(wx-config --libs base core aui --cppflags)

clean:
	rm epilogue