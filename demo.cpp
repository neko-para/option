#include "option.h"
#include <unistd.h>
#include <fcntl.h>

using namespace OPTION;

int main(int argc, char* argv[]) {
	auto option = Option(Chooser(
		Option(Req("touch"), Req("file", true), (Function)[](const ParamList& p) {
			if (access(p[0].c_str(), F_OK) < 0) {
				creat(p[0].c_str(), 644);
			}
		}),
		Option(Req("rm"), Req("file", true), (Function)[](const ParamList& p) {
			unlink(p[0].c_str());
		})
	));
	option->start(argc, argv);
}