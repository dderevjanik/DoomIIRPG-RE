#include "DataNode.h"
#include <yaml-cpp/yaml.h>

// --- DataNode::Impl ---

struct DataNode::Impl {
	YAML::Node node;
	Impl() {}
	Impl(const YAML::Node& n) : node(n) {}
};

// --- DataNode ---

DataNode::DataNode() : impl(std::make_shared<Impl>()) {}
DataNode::~DataNode() = default;
DataNode::DataNode(const DataNode& other) = default;
DataNode& DataNode::operator=(const DataNode& other) = default;
DataNode::DataNode(DataNode&& other) noexcept = default;
DataNode& DataNode::operator=(DataNode&& other) noexcept = default;

DataNode DataNode::fromYAML(const YAML::Node& node) {
	DataNode dn;
	dn.impl = std::make_shared<Impl>(node);
	return dn;
}

DataNode DataNode::loadFile(const char* path) {
	try {
		YAML::Node node = YAML::LoadFile(path);
		return fromYAML(node);
	} catch (const YAML::Exception&) {
		return DataNode();
	}
}

int DataNode::asInt(int def) const {
	if (!impl || !impl->node.IsDefined() || impl->node.IsNull()) return def;
	try { return impl->node.as<int>(def); }
	catch (...) { return def; }
}

long long DataNode::asLongLong(long long def) const {
	if (!impl || !impl->node.IsDefined() || impl->node.IsNull()) return def;
	try { return impl->node.as<long long>(def); }
	catch (...) { return def; }
}

unsigned long DataNode::asULong(unsigned long def) const {
	if (!impl || !impl->node.IsDefined() || impl->node.IsNull()) return def;
	try { return impl->node.as<unsigned long>(def); }
	catch (...) { return def; }
}

float DataNode::asFloat(float def) const {
	if (!impl || !impl->node.IsDefined() || impl->node.IsNull()) return def;
	try { return impl->node.as<float>(def); }
	catch (...) { return def; }
}

bool DataNode::asBool(bool def) const {
	if (!impl || !impl->node.IsDefined() || impl->node.IsNull()) return def;
	try { return impl->node.as<bool>(def); }
	catch (...) { return def; }
}

std::string DataNode::asString(const std::string& def) const {
	if (!impl || !impl->node.IsDefined() || impl->node.IsNull()) return def;
	try { return impl->node.as<std::string>(def); }
	catch (...) { return def; }
}

DataNode DataNode::operator[](const char* key) const {
	if (!impl || !impl->node.IsDefined() || !impl->node.IsMap()) return DataNode();
	DataNode dn;
	dn.impl = std::make_shared<Impl>(impl->node[key]);
	return dn;
}

DataNode DataNode::operator[](const std::string& key) const {
	return (*this)[key.c_str()];
}

DataNode DataNode::operator[](int index) const {
	if (!impl || !impl->node.IsDefined()) return DataNode();
	DataNode dn;
	dn.impl = std::make_shared<Impl>(impl->node[index]);
	return dn;
}

DataNode::operator bool() const {
	return impl && impl->node.IsDefined() && !impl->node.IsNull();
}

bool DataNode::isMap() const {
	return impl && impl->node.IsMap();
}

bool DataNode::isScalar() const {
	return impl && impl->node.IsScalar();
}

bool DataNode::isSequence() const {
	return impl && impl->node.IsSequence();
}

int DataNode::size() const {
	if (!impl || !impl->node.IsDefined()) return 0;
	return (int)impl->node.size();
}

DataNode DataNode::clone() const {
	if (!impl) return DataNode();
	DataNode dn;
	dn.impl = std::make_shared<Impl>(YAML::Clone(impl->node));
	return dn;
}

DataNode DataNode::merge(const DataNode& base, const DataNode& overlay, const char* skipKey) {
	DataNode result = base.clone();
	if (!overlay.impl || !overlay.impl->node.IsDefined()) return result;
	if (!result.impl) result.impl = std::make_shared<Impl>();
	for (auto it = overlay.impl->node.begin(); it != overlay.impl->node.end(); ++it) {
		std::string key = it->first.as<std::string>();
		if (skipKey && key == skipKey) continue;
		result.impl->node[key] = it->second;
	}
	return result;
}

DataNode DataNode::mergeDeep(const DataNode& base, const DataNode& overlay) {
	DataNode result = base.clone();
	if (!overlay.impl || !overlay.impl->node.IsDefined()) return result;
	if (!result.impl) result.impl = std::make_shared<Impl>();
	for (auto it = overlay.impl->node.begin(); it != overlay.impl->node.end(); ++it) {
		std::string key = it->first.as<std::string>();
		if (result.impl->node[key].IsMap() && it->second.IsMap()) {
			// Merge map contents instead of overwriting
			for (auto sub = it->second.begin(); sub != it->second.end(); ++sub) {
				result.impl->node[key][sub->first] = sub->second;
			}
		} else {
			result.impl->node[key] = it->second;
		}
	}
	return result;
}

// --- DataNode::Iterator::Impl ---

struct DataNode::Iterator::Impl {
	YAML::Node::const_iterator it;
	YAML::Node::const_iterator end;
	bool isEnd;

	Impl() : isEnd(true) {}
	Impl(YAML::Node::const_iterator begin, YAML::Node::const_iterator end)
		: it(begin), end(end), isEnd(begin == end) {}
};

// --- DataNode::Iterator ---

DataNode::Iterator::Iterator() : impl(std::make_shared<Impl>()) {}
DataNode::Iterator::~Iterator() = default;
DataNode::Iterator::Iterator(const Iterator& other) = default;
DataNode::Iterator& DataNode::Iterator::operator=(const Iterator& other) = default;

DataNode DataNode::Iterator::key() const {
	if (impl && !impl->isEnd) {
		return DataNode::fromYAML(impl->it->first);
	}
	return DataNode();
}

DataNode DataNode::Iterator::value() const {
	if (impl && !impl->isEnd) {
		// yaml-cpp iterator_value inherits from both Node and pair<Node,Node>.
		// For maps, ->second holds the value. For sequences, ->second is a
		// zombie (IsDefined()==false) and the Node base (*it) is the element.
		const YAML::Node& second = impl->it->second;
		if (second.IsDefined()) {
			return DataNode::fromYAML(second);
		}
		return DataNode::fromYAML(*impl->it);
	}
	return DataNode();
}

DataNode::Iterator& DataNode::Iterator::operator++() {
	if (impl && !impl->isEnd) {
		++(impl->it);
		impl->isEnd = (impl->it == impl->end);
	}
	return *this;
}

bool DataNode::Iterator::operator!=(const Iterator& other) const {
	// End iterators: both end = equal, one end one not = not equal
	bool thisEnd = !impl || impl->isEnd;
	bool otherEnd = !other.impl || other.impl->isEnd;
	if (thisEnd && otherEnd) return false;
	if (thisEnd != otherEnd) return true;
	return impl->it != other.impl->it;
}

DataNode::Iterator DataNode::begin() const {
	if (!impl || !impl->node.IsDefined() || impl->node.IsNull() || impl->node.IsScalar()) {
		return Iterator();
	}
	Iterator it;
	it.impl = std::make_shared<Iterator::Impl>(impl->node.begin(), impl->node.end());
	return it;
}

DataNode::Iterator DataNode::end() const {
	return Iterator(); // sentinel
}
