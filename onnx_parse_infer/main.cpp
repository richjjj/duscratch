////////////////////////////////////////////////////////////////////////////////////////////////////
#include <vector>
#include <string>
#include <map>
#include <memory>
#include <iostream>
#include <fstream>
#include "onnx.proto3.pb.h"


//  spconv_infer1.py
class BaseNode : std::enable_shared_from_this<BaseNode> {

public:
	BaseNode() {}
	virtual ~BaseNode() {}

	virtual void update() {}

};

class Tensor : std::enable_shared_from_this<Tensor> {
public:
	Tensor(const std::string& name, BaseNode* parent = nullptr) :
		name_(name),
		value_(0.f)
	{
		parent_ = parent;
	}

	~Tensor() {
	}


	void update() {
		if (this->parent_ != nullptr)
		{
			this->parent_->update();
		}
	}
public:
	std::string name_;
	float value_;
	BaseNode* parent_;  // 父类指针
};

class Node : public BaseNode, std::enable_shared_from_this<Node> {
public:
	Node(const std::string& name, const std::string& op_type) :
		name_(name),
		op_type_(op_type)
	{
		input_.clear();
		//output_.clear();
	}
	virtual  ~Node() {}
	void update() {
		if (!this->is_computed_) {
			this->is_computed_ = true;

			for (const auto& x : input_)
			{
				x->update();
			}

			this->forward();
		}
	}

	virtual void forward() {};

public:
	std::string name_{ "" };
	std::string op_type_{ "" };
	bool is_computed_{ false };
	std::vector<Tensor*> input_;
	Tensor* output_{ nullptr };
};

class SparseConvolution : public Node {
public:
	SparseConvolution(const std::string& name, Tensor* x, const std::string& attr) : Node(name, "SparseConvolution") {
		attributes_ = attr;
		this->input_.push_back(x);
		std::string n = name + ".ouput";
		auto parent = dynamic_cast<BaseNode*>(this);
		this->output_ = new Tensor(n, parent);
	}
	~SparseConvolution() {
		if (this->output_)
		{
			delete output_;
			output_ = nullptr;
		}
	}


	void forward() override {
		this->output_->value_ = this->input_[0]->value_ * 0.5;
		printf("Do %s x[%f] * 0.5, output = %f\n", this->op_type_.c_str(), this->input_[0]->value_, this->output_->value_);
	}

private:
	std::string attributes_;
};

class ReLU : public Node {
public:
	ReLU(const std::string& name, Tensor* x) :
		Node(name, "ReLU") {
		this->input_.push_back(x);
		std::string n = name + ".ouput";
		auto parent = dynamic_cast<BaseNode*>(this);
		this->output_ = new Tensor(n, parent);
	}
	~ReLU() {
		if (this->output_)
		{
			delete output_;
			output_ = nullptr;
		}
	}


	void forward() override {
		this->output_->value_ = std::max(0.f, this->input_[0]->value_);
		printf("Do %s max(0, x[%f]), output = %f\n", this->op_type_.c_str(), this->input_[0]->value_, this->output_->value_);
	}
};

class Add : public Node {
public:
	Add(const std::string& name, Tensor* a, Tensor* b) :
		Node(name, "Add") {
		this->input_.push_back(a);
		this->input_.push_back(b);
		std::string n = name + ".ouput";
		auto parent = dynamic_cast<BaseNode*>(this);
		this->output_ = new Tensor(n, parent);
	}
	~Add() {
		if (this->output_)
		{
			delete output_;
			output_ = nullptr;
		}
	}


	void forward() override {
		this->output_->value_ = this->input_[0]->value_ + this->input_[1]->value_;
		printf("Do %s a[%f] + b[%f], output = %f\n", this->op_type_.c_str(), this->input_[0]->value_, this->input_[1]->value_, this->output_->value_);
	}
};

class BatchNormalization : public Node {
public:
	BatchNormalization(const std::string& name, Tensor* x, const std::string& attr) :
		Node(name, "BatchNormalization") {
		attributes_ = attr;
		this->input_.push_back(x);
		std::string n = name + ".ouput";
		auto parent = dynamic_cast<BaseNode*>(this);
		this->output_ = new Tensor(n, parent);
	}
	~BatchNormalization() {
		if (this->output_)
		{
			delete output_;
			output_ = nullptr;
		}
	}


	void forward() override {
		this->output_->value_ = this->input_[0]->value_ + 0.1;
		printf("Do %s x[%f] +0.1, output = %f\n", this->op_type_.c_str(), this->input_[0]->value_, this->output_->value_);
	}

private:
	std::string attributes_;
};

class Engine {
public:
	Engine() {}
	~Engine() {
		// 析构
		for (int i = 0; i < this->inputs_.size(); ++i)
		{
			if (this->inputs_[i])
			{
				delete this->inputs_[i];
				this->inputs_[i] = nullptr;
			}
		}
		for (int i = 0; i < this->outputs_.size(); ++i)
		{
			if (this->outputs_[i])
			{
				delete this->outputs_[i];
				this->outputs_[i] = nullptr;
			}
		}
		for (int i = 0; i < this->nodes_.size(); ++i)
		{
			if (this->nodes_[i])
			{
				delete this->nodes_[i];
				this->nodes_[i] = nullptr;
			}
		}

	}

	Tensor* add_input(const std::string& name)
	{
		auto x = new Tensor(name);
		this->inputs_.push_back(x);
		return x;
	}
	Tensor* mark_output(Tensor* x) {
		this->outputs_.push_back(x);
		return x;
	}

	Node* add_spconv(const std::string& name, Tensor* x, std::string attributes = "") {
		auto spc = new SparseConvolution(name, x, attributes);
		this->nodes_.push_back(spc);
		return spc;
	}

	Node* add_relu(const std::string& name, Tensor* x) {
		auto relu = new ReLU(name, x);
		this->nodes_.push_back(relu);
		return relu;
	}

	Node* add_add(const std::string& name, Tensor* a, Tensor* b) {
		auto add = new Add(name, a, b);
		this->nodes_.push_back(add);
		return add;
	}

	Node* add_bn(const std::string& name, Tensor* x, std::string attributes = "") {
		auto bn = new BatchNormalization(name, x, attributes);
		this->nodes_.push_back(bn);
		return bn;
	}

	float forward(float x) {
		for (const auto& n : this->nodes_)
		{
			n->is_computed_ = false;
		}

		this->inputs_[0]->value_ = x;
		this->outputs_[0]->update();
		return this->outputs_[0]->value_;
	}

private:
	std::vector<Tensor*> inputs_;
	std::vector<Tensor*> outputs_;
	std::vector <Node*> nodes_;
};

Engine* load_engine(const std::string& onnx_file) {
	auto engine = new Engine();
	onnx::ModelProto model;
	std::ifstream in(onnx_file, std::ios_base::binary);
	model.ParseFromIstream(&in);
	in.close();
	//std::cout << model.graph().input().size() << "\n";

	std::map<std::string, Tensor*> name_to_tensor;
	for (const auto& i : model.graph().input())
	{
		auto x = engine->add_input(i.name());
		name_to_tensor[x->name_] = x;
	}

	for (const auto& n : model.graph().node())
	{
		if (n.op_type() == "SparseConvolution") {
			auto layer = engine->add_spconv(n.name(), name_to_tensor[n.input()[0]], "");
			name_to_tensor[n.output()[0]] = layer->output_;
		}
		else if (n.op_type() == "BatchNormalization") {
			auto layer = engine->add_bn(n.name(), name_to_tensor[n.input()[0]], "");
			name_to_tensor[n.output()[0]] = layer->output_;
		}
		else if (n.op_type() == "Relu") {
			auto layer = engine->add_relu(n.name(), name_to_tensor[n.input()[0]]);
			name_to_tensor[n.output()[0]] = layer->output_;
		}
		else if (n.op_type() == "Add") {
			auto layer = engine->add_add(n.name(), name_to_tensor[n.input()[0]], name_to_tensor[n.input()[1]]);
			name_to_tensor[n.output()[0]] = layer->output_;
		}
	}

	for (const auto& o : model.graph().output())
	{
		engine->mark_output(name_to_tensor[o.name()]);
	}

	return engine;
}

int main() {

	std::string onnx = R"(D:\LearningCodes\GithubRepo\shouxieAI\spconv-onnx\code\code\scn.onnx)";
	auto engine = load_engine(onnx); 
	unsigned int i = 0;
	while (i < 10000)
	{
		++i;
		std::cout << "result = " << engine->forward(1.0) << std::endl;
	}

	return 0;
}
