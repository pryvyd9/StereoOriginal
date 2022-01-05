#pragma once 

#include <stdlib.h>
#include <chrono>
#include <iostream>
#include "TemplateExtensions.hpp"
#include <sstream>
#include <string>
#include <vector>
#include <set>
#include <functional>
#include <map>
#include <glm/vec3.hpp>

#include <fstream>
#include <filesystem>// C++17 standard header file name

#include <thread>
namespace fs = std::filesystem;

template<typename T>
int find(const std::vector<T>& source, const T& item) {
	for (size_t i = 0; i < source.size(); i++)
		if (source[i] == item)
			return i;

	return -1;
}
template<typename T>
int find(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	for (size_t i = 0; i < source.size(); i++)
		if (condition(source[i]))
			return i;

	return -1;
}
//template<typename T>
//int find(std::vector<T*>& source, std::function<bool(T*)> condition) {
//	for (size_t i = 0; i < source.size(); i++)
//		if (condition(source[i]))
//			return i;
//
//	return -1;
//}
template<typename T>
int findBack(std::vector<T*>& source, std::function<bool(T*)> condition) {
	if (source.empty())
		return -1;

	for (int i = source.size() - 1; i >= 0; i--)
		if (condition(source[i]))
			return i;

	return -1;
}

template<typename T>
std::vector<int> findAll(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	std::vector<int> indices;
	for (size_t i = 0; i < source.size(); i++)
		if (condition(source[i]))
			indices.push_back(i);

	return indices;
}
template<typename T>
std::vector<int> findAllBack(const std::vector<T>& source, std::function<bool(const T&)> condition) {
	std::vector<int> indices;
	for (size_t i = source.size(); i-- != 0; )
		if (condition(source[i]))
			indices.push_back(i);

	return indices;
}
template<typename T>
bool exists(const std::set<T>& source, const T& item) {
	return source.find(item) != source.end();
}
template<typename T>
bool exists(const std::set<const T>& source, const T& item) {
	return source.find(item) != source.end();
}
template<typename K, typename T>
bool exists(const std::set<K>& source, const T& item, std::function<T(const K&)> selector) {
	for (auto o : source)
		if (selector(o) == item)
			return true;

	return false;
}

template<typename T>
bool exists(const std::vector<T>& source, const T& item) {
	return std::find(source.begin(), source.end(), item) != source.end();
}
template<typename T>
bool exists(const std::list<T>& source, const T& item) {
	return std::find(source.begin(), source.end(), item) != source.end();
}
template<typename K, typename V>
bool keyExists(const std::set<K,V>& source, const K& item) {
	return source.find(item) != source.end();
}

class Path {
	fs::path path;
	std::string pathBuffer;
public:
	const fs::path& get() const {
		return path;
	}
	std::string& getBuffer() {
		return pathBuffer;
	}

	Path() {}
	Path(fs::path n) {
		apply(n);
	}
	void apply() {
		apply(pathBuffer);
	}
	void apply(fs::path n) {
		path = fs::absolute(n);
		pathBuffer = path.u8string();
	}

	bool isSome() {
		return !pathBuffer.empty();
	}

	std::string join(Path path) {
		auto last = pathBuffer[pathBuffer.size() - 1];

		if (last == '/' || last == '\\')
			return pathBuffer + path.getBuffer();

		return pathBuffer + '/' + path.getBuffer();
	}
	fs::path joinPath(const std::string& path) {
		auto last = pathBuffer[pathBuffer.size() - 1];

		if (last == '/' || last == '\\')
			return fs::absolute(pathBuffer + path);

		return fs::absolute(pathBuffer + '/' + path);
	}
};

class Time {
	static const int timeLogSize = 5;

	static std::chrono::steady_clock::time_point* GetBegin() {
		static std::chrono::steady_clock::time_point instance;
		return &instance;
	}
	static size_t* GetDeltaTimeMicroseconds() {
		static size_t instance;
		return &instance;
	}

	static std::vector<size_t>& TimeLog() {
		static std::vector<size_t> v(timeLogSize);
		return v;
	}

	static void UpdateTimeLog(size_t v) {
		static int i = 0;
		TimeLog()[i] = v;
		i++;
		if (i >= timeLogSize)
			i = 0;
	}
public:
	static void UpdateFrame() {
		auto end = std::chrono::steady_clock::now();
		*GetDeltaTimeMicroseconds() = std::chrono::duration_cast<std::chrono::microseconds>(end - *GetBegin()).count();
		*GetBegin() = end;

		UpdateTimeLog(*GetDeltaTimeMicroseconds());
	}
	static int GetFrameRate() {
		return round(1 / GetDeltaTime());
	}
	static int GetAverageFrameRate() {
		return round(1 / GetAverageDeltaTime());
	}
	static float GetDeltaTime() {
		return (float)*GetDeltaTimeMicroseconds() / 1e6;
	}
	static float GetAverageDeltaTime() {
		size_t t = 0;
		for (auto a : TimeLog())
			t += a;

		return (float)t / 1e6 / timeLogSize;
	}
	static size_t GetTime() {
		return std::chrono::time_point_cast<std::chrono::milliseconds>(*GetBegin()).time_since_epoch().count();
	}
	static const size_t& GetAppStartTime() {
		static size_t v = std::chrono::time_point_cast<std::chrono::milliseconds>(std::chrono::system_clock::now()).time_since_epoch().count();
		return v;
	}
	static std::string GetTimeFormatted(const std::string& format = "%Y-%m-%d %H:%M:%S") {
		std::chrono::system_clock::time_point now = std::chrono::system_clock::now();
		std::time_t now_c = std::chrono::system_clock::to_time_t(now);

#pragma warning(push)
#pragma warning(disable: 4996)
		std::tm tm = *std::gmtime(&now_c);
#pragma warning(pop)
		
		std::stringstream ss;
		ss << std::put_time(&tm, format.c_str());
		return ss.str();
	}

};

class Log {
public:
	struct Context {
		std::string contextName = "";
		std::string logFileName = "";
		std::string message = "";
		std::string level = "";
	};
private:
	template<typename T>
	using isOstreamable = decltype(std::declval<std::ostringstream>() << std::declval<T>());

	template<typename T>
	static constexpr bool isOstreamableT = is_detected_v<isOstreamable, T>;


	Context context;
	std::function<void(const Context&)> sink = Sink();
	
	template<typename T, std::enable_if_t<isOstreamableT<T>> * = nullptr>
	static std::string ToString(const T& message) {
		std::ostringstream ss;
		ss << message;
		return ss.str();
	}
	static std::string ToString(const glm::vec3& message) {
		std::ostringstream ss;
		ss << "(" << message.x << ";" << message.y << ";" << message.z << ")";
		return ss.str();
	}

public:
	template<typename T>
	static const Log For(std::string logFileName = LogFileName(), std::function<void(const Context&)> sink = Sink()) {
		Log log;
		log.context.contextName = typeid(T).name();
		log.context.logFileName = logFileName;
		log.sink = sink;
		return log;
	}

	template<typename... T>
	void Error(const T&... message) const {
		Error((Log::ToString(message) + ...));
	}
	template<typename... T>
	void Warning(const T&... message) const {
		Warning((Log::ToString(message) + ...));
	}
	template<typename... T>
	void Information(const T&... message) const {
		Information((Log::ToString(message) + ...));
	}


	void Error(const std::string& message) const {
		Context c = context;
		c.level = "Error";
		c.message = message;
		sink(c);
	}
	void Warning(const std::string& message) const {
		Context c = context;
		c.level = "Warning";
		c.message = message;
		sink(c);
	}
	void Information(const std::string& message) const {
		Context c = context;
		c.level = "Information";
		c.message = message;
		sink(c);
	}

	static std::string& LogFileName() {
		static std::string v = "defaultLog";
		return v;
	}
	static std::function<void(const Context&)>& Sink() {
		static std::function<void(const Context&)> v = [](const Context&) {};
		return v;
	}

	static void FileSink(const Context& v) {
		static Path logDirectory("logs/");
		static std::string startTime = Time::GetTimeFormatted("%Y%m%d%H%M%S");
		if (!fs::is_directory(logDirectory.get()) || !fs::exists(logDirectory.get()))
			fs::create_directory(logDirectory.get());

		std::stringstream ss;
		ss << Time::GetTimeFormatted() << "[" << v.level << "]" << "(" << v.contextName << ")" << v.message << std::endl;
		auto log = ss.str();

		std::ofstream f(logDirectory.joinPath(v.logFileName + startTime), std::ios_base::app);
		f << log;
		f.close();
	}
	static void ConsoleSink(const Context& v) {
		std::cout << "[" << v.level << "]" << "(" << v.contextName << ")" << v.message << std::endl;
	}
};

class Command {
	static std::list<Command*>& GetQueue() {
		static auto queue = std::list<Command*>();
		return queue;
	}
protected:
	bool isReady = false;
	// Defines wether command must be deleted after execution.
	// false = will be deleted. 
	// true  = will not be deleted.
	// Persistent commands are executed once every frame 
	// until mustPersist is set to false.
	bool mustPersist = false;
	virtual bool Execute() = 0;
public:
	Command() {
		GetQueue().push_back(this);
	}
	static bool ExecuteAll() {
		std::list<Command*> deleteQueue;
		for (auto command : GetQueue())
			if (command->isReady) {
				if (!command->Execute())
					return false;

				if (!command->mustPersist)
					deleteQueue.push_back(command);
			}

		for (auto command : deleteQueue) {
			GetQueue().remove(command);
			delete command;
		}

		return true;
	}
};

class FuncCommand : Command {
	bool shouldAbort = false;
protected:
	virtual bool Execute() {
		if (!shouldAbort)
			func();

		return true;
	};
public:
	FuncCommand() {
		isReady = true;
	}

	void Abort() {
		shouldAbort = true;
	}

	std::function<void()> func;
};

template<typename...T>
class IEvent {
	FuncCommand* modifyHandlersCommand = nullptr;
	std::map<size_t, std::function<void(const T&...)>> handlersToBeAdded;
	std::vector<size_t> handlersToBeRemoved;

	void EnsureModifyHandlersCommand() {
		if (!modifyHandlersCommand) {
			modifyHandlersCommand = new FuncCommand();
			modifyHandlersCommand->func = [&] {
				handlers.insert(handlersToBeAdded.begin(), handlersToBeAdded.end());
				for (auto h : handlersToBeRemoved)
					handlers.erase(h);

				modifyHandlersCommand = nullptr;
				handlersToBeAdded.clear();
				handlersToBeRemoved.clear();
			};
		}
	}
protected:
	std::map<size_t, std::function<void(const T&...)>> handlers;
public:
	size_t AddHandler(std::function<void(const T&...)> func) {
		static size_t id = 0;

		handlersToBeAdded[id] = func;
		EnsureModifyHandlersCommand();

		return id++;
	}

	void RemoveHandler(size_t v) {
		handlersToBeRemoved.push_back(v);
		EnsureModifyHandlersCommand();
	}

	size_t operator += (std::function<void(const T&...)> func) {
		return AddHandler(func);
	}
	void operator -= (size_t v) {
		RemoveHandler(v);
	}

	~IEvent() {
		// Commands will be executed with skipping the assigned func 
		// and then will be deleted so we only need to abort them.
		if (modifyHandlersCommand)
			modifyHandlersCommand->Abort();
	}

	void AddHandlersFrom(const IEvent<T...>& o) {
		handlersToBeAdded.insert(o.handlers.begin(), o.handlers.end());
		handlersToBeAdded.insert(o.handlersToBeAdded.begin(), o.handlersToBeAdded.end());
		handlersToBeRemoved.insert(handlersToBeRemoved.end(), o.handlersToBeRemoved.begin(), o.handlersToBeRemoved.end());

		EnsureModifyHandlersCommand();
	}
};
template<typename...T>
class Event : public IEvent<T...> {
public:
	void Invoke(const T&... vs) {
		for (auto h : IEvent<T...>::handlers)
			h.second(vs...);
	}
	IEvent<T...>& Public() {
		return *this;
	}
};

namespace P {
	// PropertyNode
	template<typename T>
	class PN {
		T value;
		Event<T> changed;
	public:
		const T& Get() const {
			return value;
		}
		T& Get() {
			return value;
		}
		void Set(const T& v) {
			value = v;
			changed.Invoke(v);
		}
		IEvent<T>& OnChanged() {
			return changed;
		}
		bool IsAssigned() {
			return true;
		}
	};
	template<typename T>
	class PN<T*> {
		T* value = nullptr;
		Event<T> changed;
	public:
		const T& Get() const {
			return *value;
		}
		T& Get() {
			return *value;
		}
		void Set(T* v) {
			value = v;
			changed.Invoke(*v);
		}
		IEvent<T>& OnChanged() {
			return changed;
		}
		bool IsAssigned() {
			return value != nullptr;
		}
	};
	template<typename R, typename ...T>
	class PN<std::function<R(T...)>> {
		std::function<R(T...)> value;
		Event<std::function<R(T...)>> changed;
		bool isAssigned = false;
	public:
		const std::function<R(T...)>& Get() const {
			return value;
		}
		void Set(const std::function<R(T...)>& v) {
			value = v;
			isAssigned = true;
			changed.Invoke(v);
		}
		IEvent<std::function<R(T...)>>& OnChanged() {
			return changed;
		}
		bool IsAssigned() {
			return isAssigned;
		}
		R operator()(T... vs) {
			return value(vs...);
		}
	};

	enum class Source {
		GUI,
		NonGUI,
		Keyboard,
		Mouse,
		None,
	};

	// MultiSourcePropertyNode
	// Handles user input.
	// Unlike simple properties its events are fired once per frame
	// ensuring only the most prioritized input triggers the event.
	template<typename T>
	class MSPN {
	public:
		template<typename T>
		struct EventArgs{
			Source source;
			T newValue;
		};
	private:
		T value;
		Event<EventArgs<T>> changed;
		EventArgs<T> eventArgs;
		FuncCommand* cmd = nullptr;
	public:
	
		const T& Get() const {
			return value;
		}
		T& Get() {
			return value;
		}
		void Set(const T& v, Source source) {
			value = v;
			
			if (eventArgs.source < source) {
				eventArgs.source = source;
				eventArgs.newValue = v;
			}
	
			if (!cmd) {
				cmd = new FuncCommand();
				cmd->func = [&changed, &eventArgs, &cmd] {
					changed.Invoke(eventArgs);
					cmd = nullptr;
					eventArgs.source = Source::None;
				};
			}
		}
		IEvent<EventArgs<T>>& OnChanged() {
			return changed;
		}
		bool IsAssigned() {
			return true;
		}
	};


	// ReadonlyProperty
	template<typename T, template<typename> typename Node, typename EventArgs>
	class RP {
	protected:
		using _RP = RP<T, Node, EventArgs>;
		using _Node = Node<T>;

		std::shared_ptr<_Node> node = std::make_shared<_Node>();
		RP(const std::shared_ptr<_Node>& node) {
			this->node = node;
		}
		void ReplaceNode(const std::shared_ptr<_Node>& node) {
			node->OnChanged().AddHandlersFrom(this->node->OnChanged());
			this->node = node;
		}
	public:
		RP() {}
		RP(const T& o) {
			node->Set(o);
		}

		bool IsAssigned() {
			return node->IsAssigned();
		}

		const T& Get() const {
			return node->Get();
		}

		_RP CloneReadonly() {
			return new _RP(node);
		}

		IEvent<EventArgs>& OnChanged() {
			return node->OnChanged();
		}

		_RP& operator=(const _RP&) = delete;
		const T* operator->() const {
			return &Get();
		}
		void operator<<=(const _RP& v) {
			ReplaceNode(v.node);
		}
	};
	template<typename T, template<typename> typename Node, typename EventArgs>
	class RP<T*, Node, EventArgs> {
	protected:
		using _RP = RP<T*, Node, EventArgs>;
		using _Node = Node<T*>;

		std::shared_ptr<_Node> node = std::make_shared<_Node>();
		RP(const std::shared_ptr<_Node>& node) {
			this->node = node;
		}
		void ReplaceNode(const std::shared_ptr<_Node>& node) {
			node->OnChanged().AddHandlersFrom(this->node->OnChanged());
			this->node = node;
		}
	public:
		RP() {}
		RP(T* o) {
			node->Set(o);
		}

		bool IsAssigned() {
			return node->IsAssigned();
		}

		const T& Get() const {
			return node->Get();
		}

		_RP CloneReadonly() {
			return new _RP(node);
		}

		IEvent<EventArgs>& OnChanged() {
			return node->OnChanged();
		}

		_RP& operator=(const _RP&) = delete;
		const T* operator->() const {
			return &Get();
		}
		void operator<<=(const _RP& v) {
			ReplaceNode(v.node);
		}
	};
	template<typename R, template<typename> typename Node, typename EventArgs, typename ...T>
	class RP<std::function<R(T...)>, Node, EventArgs> {
	protected:
		using _RP = RP<std::function<R(T...)>, Node, EventArgs>;
		using _Node = Node<std::function<R(T...)>>;

		std::shared_ptr<_Node> node = std::make_shared<_Node>();
		RP(const std::shared_ptr<_Node>& node) {
			this->node = node;
		}
		void ReplaceNode(const std::shared_ptr<_Node>& node) {
			node->OnChanged().AddHandlersFrom(this->node->OnChanged());
			this->node = node;
		}
	public:
		RP() {}
		RP(std::function<R(T...)> o) {
			node->Set(o);
		}

		bool IsAssigned() {
			return node->IsAssigned();
		}

		const std::function<R(T...)>& Get() const {
			return node->Get();
		}

		_RP CloneReadonly() {
			return new RP(node);
		}

		IEvent<EventArgs>& OnChanged() {
			return node->OnChanged();
		}

		_RP& operator=(const _RP&) = delete;
		R operator()(T... vs) const {
			return Get()(vs...);
		}
		void operator<<=(const _RP& v) {
			ReplaceNode(v.node);
		}
	};

	// NonAssignProperty
	template<typename T, template<typename> typename Node, typename EventArgs>
	class NAP : public RP<T, Node, EventArgs> {
	protected:
		using _RP = RP<T, Node, EventArgs>;
		using _NAP = NAP<T, Node, EventArgs>;
		using _Node = Node<T>;

		NAP(const _Node& node) {
			_RP::node = node;
		}
	public:
		NAP() {}
		NAP(const T& o) : _RP(o) {}

		T& Get() {
			return _RP::node->Get();
		}

		_NAP CloneNonAssign() {
			return new _NAP(RP::node);
		}

		_NAP& operator=(const _NAP&) = delete;
		T* operator->() {
			return &Get();
		}
		void operator<<=(const _NAP& v) {
			_RP::ReplaceNode(v.node);
		}
	};
	template<typename T, template<typename> typename Node, typename EventArgs>
	class NAP<T*, Node, EventArgs> : public RP<T*, Node, EventArgs> {
	protected:
		using _RP = RP<T*, Node, EventArgs>;
		using _NAP = NAP<T*, Node, EventArgs>;
		using _Node = Node<T*>;

		NAP(const _Node& node) {
			_RP::node = node;
		}
	public:
		NAP() {}
		NAP(T* o) : _RP(o) {}

		T& Get() {
			return _RP::node->Get();
		}

		_NAP CloneNonAssign() {
			return new _NAP(RP::node);
		}

		_NAP& operator=(const _NAP&) = delete;
		T* operator->() {
			return &Get();
		}
		void operator<<=(const _NAP& v) {
			_RP::ReplaceNode(v.node);
		}
	};
	template<typename R, template<typename> typename Node, typename EventArgs, typename ...T>
	class NAP<std::function<R(T...)>, Node, EventArgs> : public RP<std::function<R(T...)>, Node, EventArgs> {
	protected:
		using _RP = RP<std::function<R(T...)>, Node, EventArgs>;
		using _NAP = NAP<std::function<R(T...)>, Node, EventArgs>;
		using _Node = Node<std::function<R(T...)>>;

		NAP(const _Node& node) {
			_RP::node = node;
		}
	public:
		NAP() {}
		NAP(const std::function<R(T...)>& o) : RP(o) {}

		_NAP CloneNonAssign() {
			return new _NAP(_RP::node);
		}

		_NAP& operator=(const _NAP&) = delete;
		void operator<<=(const _NAP& v) {
			_RP::ReplaceNode(v.node);
		}
	};

	// Property
	template<typename T, template<typename> typename Node, typename EventArgs>
	class P : public NAP<T, Node, EventArgs> {
	protected:
		using _RP = RP<T, Node, EventArgs>;
		using _NAP = NAP<T, Node, EventArgs>;
		using _P = P<T, Node, EventArgs>;

	public:
		P() {}
		P(const T& o) : _NAP(o) {}
		P(const P&) = delete;


		_P& operator=(const _P&) = delete;
		_P& operator=(const T& v) {
			_RP::node->Set(v);
			return *this;
		}
		void operator<<=(const _P& v) {
			_RP::ReplaceNode(v.node);
		}
	};
	template<typename T, template<typename> typename Node, typename EventArgs>
	class P<T*, Node, EventArgs> : public NAP<T*, Node, EventArgs> {
	protected:
		using _RP = RP<T*, Node, EventArgs>;
		using _NAP = NAP<T*, Node, EventArgs>;
		using _P = P<T*, Node, EventArgs>;

	public:
		P() {}
		P(T* o) : _NAP(o) {}

		_P& operator=(const _P&) = delete;
		_P& operator=(T* v) {
			_RP::node->Set(v);
			return *this;
		}
		void operator<<=(const _P& v) {
			_RP::ReplaceNode(v.node);
		}
	};
	template<typename R, template<typename> typename Node, typename EventArgs, typename ...T>
	class P<std::function<R(T...)>, Node, EventArgs> : public NAP<std::function<R(T...)>, Node, EventArgs> {
	protected:
		using _RP = RP<std::function<R(T...)>, Node, EventArgs>;
		using _NAP = NAP<std::function<R(T...)>, Node, EventArgs>;
		using _P = P<std::function<R(T...)>, Node, EventArgs>;

	public:
		P() {}
		P(const std::function<R(T...)>& o) : _NAP(o) {}
		P(const _P&) = delete;


		_P& operator=(const _P&) = delete;
		_P& operator=(const std::function<R(T...)>& v) {
			_RP::node->Set(v);
			return *this;
		}
		void operator<<=(const _P& v) {
			_RP::ReplaceNode(v.node);
		}
	};
};

template<typename T>
using ReadonlyProperty = P::RP<T, P::PN, T>;

template<typename T>
using NonAssignProperty = P::NAP<T, P::PN, T>;

template<typename T>
using Property = P::P<T, P::PN, T>;



#define StaticProperty(type,name)\
static Property<type>& name() {\
	static Property<type> v = Property<type>();\
	return v;\
}

#define StaticField(type,name)\
static type& name() {\
	static type v;\
	return v;\
}

#define StaticFieldDefault(type,name,defaultValue)\
static type& name() {\
	static type v = defaultValue;\
	return v;\
}




template<typename T>
class ValueStability {
	bool lastValueMatches = false;
	bool currentValueMatches = false;
	std::function<bool(const T&)> condition = [] { return false; };
public:
	void ApplyCondition(std::function<bool(const T&)> cond) {
		condition = cond;
	}
	void ApplyValue(const T& v) {
		currentValueMatches = condition(v);
	}

	void IsStable() {
		return lastValueMatches && currentValueMatches;
	}
	void IsBroken() {
		return lastValueMatches && !currentValueMatches;
	}
	void IsRepaired() {
		return !lastValueMatches && currentValueMatches;
	}
};