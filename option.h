#include <vector>
#include <string>
#include <set>
#include <memory>
#include <initializer_list>
#include <regex>
#include <algorithm>
#include <iostream>

namespace OPTION {
	
	using std::vector;
	using std::function;
	using std::string;
	using std::set;
	using std::shared_ptr;
	using std::initializer_list;
	using std::reverse;
	using std::regex;
	using std::regex_match;
	
	using std::cerr;
	using std::endl;
	
	typedef vector<string> ParamList;
	typedef function<void(const ParamList&)> GenericFunction;
	
	struct Wrapper {
		function<void(void)> func;
		
		Wrapper(function<void(void)> f) : func(f) {}
		void operator()(const ParamList&) {
			func();
		}
	};
	
	struct Function {
		GenericFunction func;
		Function() : func([](const ParamList&) {}) {}
		Function(GenericFunction f) : func(f) {}
		Function(function<void(void)> f) : func(Wrapper(f)) {}
		virtual ~Function() {}
		void operator()(const ParamList& p) {
			func(p);
		}
	};
	
	struct _Req {
		string name;
		bool cap;
		string reg;
		_Req(const string& n) : _Req(n, false, n) {}
		_Req(const string& n, bool c, const string& r = ".+") : name(n), cap(c), reg(r) {}
		bool capture() const {
			return cap;
		}
		bool test(const char* s) const {
			return regex_match(s, regex(reg));
		}
		string help() const {
			return name + ":/" + reg + "/";
		}
	};
	
	template <typename... Arg>
	shared_ptr<_Req> Req(const Arg&... arg) {
		return shared_ptr<_Req>(new _Req(arg...));
	}
	
	struct ArgList {
		ParamList data;
		unsigned ptr;
		
		ArgList(int argc, char** argv) {
			for (int i = 1; i < argc; ++i) {
				data.emplace_back(argv[i]);
			}
			ptr = 0;
		}
		bool check(unsigned req) const {
			return ptr + req <= data.size();
		}
		const string& peek(int offset) const {
			return data[ptr + offset];
		}
		void go(int step = 1) {
			ptr += step;
		}
	};
	
	struct OptionAction {
		virtual ~OptionAction() {}
		virtual void parse(ArgList& a, ParamList& p) = 0;
	};
	
	struct _Action : public OptionAction {
		Function func;
		
		_Action(Function f) : func(f) {}
		virtual void parse(ArgList&, ParamList& p) {
			func(p);
		}
	};

	struct OptionRequire {
		vector<shared_ptr<_Req>> req;
		
		OptionRequire() {}
		bool test(const ArgList& arg) const {
			if (!arg.check(req.size())) {
				return false;
			}
			for (unsigned i = req.size() - 1; ~i; --i) {
				if (!req[i]->test(arg.peek(req.size() - i - 1).c_str())) {
					return false;
				}
			}
			return true;
		}
		void capture(ArgList& arg, ParamList& param) {
			for (unsigned i = req.size() - 1; ~i; --i) {
				if (req[i]->capture()) {
					param.emplace_back(arg.peek(req.size() - i - 1));
				}
			}
			arg.go(req.size());
		}
		string help() const {
			string ret;
			for (unsigned i = req.size() - 1; ~i; --i) {
				ret += " " + req[i]->help();
			}
			return ret;
		}
	};
	
	struct _Option {
		shared_ptr<OptionRequire> req;
		shared_ptr<OptionAction> act;
		
		_Option() : req(new OptionRequire) {}
		template <typename... Arg>
		_Option(Function f, const Arg&... arg) : _Option(arg...) {
			act = shared_ptr<_Action>(new _Action(f));
		}
		template <typename... Arg>
		_Option(shared_ptr<_Req> r, const Arg&... arg) : _Option(arg...) {
			req->req.emplace_back(r);
		}
		_Option(shared_ptr<OptionAction> a) : req(new OptionRequire), act(a) {}
		bool test(ArgList& a) const {
			return req->test(a);
		}
		void parse(ArgList& a, ParamList& p) {
			req->capture(a, p);
			act->parse(a, p);
		}
		void start(int argc, char** argv) {
			ArgList a(argc, argv);
			ParamList p;
			if (test(a)) {
				parse(a, p);
			}
		}
		string help(string pref = "") const;
	};
	
	template <typename... Arg>
	inline shared_ptr<_Option> Option(const Arg&... arg) {
		return shared_ptr<_Option>(new _Option(arg...));
	}
	
	struct Prev : public Function {
		using Function::Function;
	};
	
	struct Post : public Function {
		using Function::Function;
	};
	
	struct _Chooser : public OptionAction {
		vector<shared_ptr<_Option>> choice;
		Function prev, post;
		
		_Chooser() {}
		template <typename... Arg>
		_Chooser(shared_ptr<_Option> o, const Arg&... arg) : _Chooser(arg...) {
			choice.emplace_back(o);
		}
		template <typename... Arg>
		_Chooser(Prev p, const Arg&... arg) : _Chooser(arg...) {
			prev = p.func;
		}
		template <typename... Arg>
		_Chooser(Post p, const Arg&... arg) : _Chooser(arg...) {
			post = p.func;
		}
		virtual void parse(ArgList& a, ParamList& p) {
			prev(p);
			for (unsigned i = choice.size() - 1; ~i; --i) {
				if (choice[i]->test(a)) {
					choice[i]->parse(a, p);
					post(p);
				}
			}
		}
		string help(string pref) const {
			string ret;
			for (unsigned i = choice.size() - 1; ~i; --i) {
				ret += choice[i]->help(pref);
			}
			return ret;
		}
	};
	
	template <typename... Arg>
	inline shared_ptr<_Chooser> Chooser(const Arg&... arg) {
		return shared_ptr<_Chooser>(new _Chooser(arg...));
	}
	
	inline string _Option::help(string pref) const {
		pref += req->help();
		string ret;
		_Chooser* p = dynamic_cast<_Chooser*>(act.get());
		if (p) {
			ret += p->help(pref);
		} else {
			ret += pref + "\n";
		}
		return ret;
	}

}
