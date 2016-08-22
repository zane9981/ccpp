#include <iostream>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>
#include <pthread.h>
#include <unistd.h>
#include <cstring>
#include <json/json.h>

#include "FCgiIO.h"

int main(int argc, char *argv[]) 
{
	int socket;

	FCGX_Init();

	socket = FCGX_OpenSocket(":8088", 500000);
	if(socket < 0) {
		std::cout << "ERROR: FCGX_OpenSocket (" << 8088 << ") returned " << std::endl;
	 	return -1;
	}

	FCGX_Request request;
	FCGX_InitRequest(&request, socket, 0);

	while(FCGX_Accept_r(&request) == 0) {
		try {
			cgicc::FCgiIO io(request);
			cgicc::Cgicc cgi(&io);
			Json::Value root;
			//Json::FastWriter writer;
			Json::StyledWriter writer;

			io << "Content-type: application/json; charset=utf-8\n\n";

			for(cgicc::const_form_iterator i = cgi.getElements().begin();
				i != cgi.getElements().end(); ++i) {
				root["param"][i->getName()] = i->getValue();
			}

			root["Client"] = cgi.getEnvironment().getRemoteAddr();
			root["method"] = cgi.getEnvironment().getRequestMethod();
			root["URI"] = cgi.getEnvironment().getScriptName();

			io << writer.write(root) << std::endl;

		} catch(const std::exception &e) {
			std::cout << "Catch Exception:" << e.what() << std::endl;
		}
		FCGX_Finish_r(&request);
	}

	return 0;
}
