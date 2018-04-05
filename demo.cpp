#include "option.h"
#include <unistd.h>
#include <fcntl.h>

using namespace OPTION::Util;

int main(int argc, char* argv[]) {
	auto option = Option(Chooser(true,
		Option(Uncapture("touch"), Capture("file"), (Function)[](ParamList& p) {
			if (access(p["file"].c_str(), F_OK) < 0) {
				creat(p["file"].c_str(), 644);
			}
		}),
		Option(Uncapture("rm"), Capture("file"), (Function)[](ParamList& p) {
			unlink(p["file"].c_str());
		})
	));
	option->start(argc, argv);
}