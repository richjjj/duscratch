#include <map>
#include <iostream>
#include <fstream>
#include <onnx/onnx_pb.h>

std::map<std::string, std::string> read_metadata_from_onnx(const onnx::ModelProto& model_proto) {
    std::map<std::string, std::string> metadata;

    const auto& model_metadata = model_proto.metadata_props();
    for (const auto& prop : model_metadata) {
        metadata.emplace(prop.key(), prop.value());
    }

    return metadata;
}

int main(int arcg, char** argv) {
    const std::string file_path = "/path/to/model.onnx";
    std::ifstream model_stream{file_path, std::ios::in | std::ios::binary};

    if (!model_stream.is_open()) {
        throw std::runtime_error("Could not open the ONNX model from file: " + file_path);
    }

    onnx::ModelProto model_proto;
    if (!model_proto.ParseFromIstream(&model_stream)) {
        throw std::runtime_error("Could not deserialize the ONNX model: " + file_path);
    } else {
        const auto onnx_metadata = read_metadata_from_onnx(model_proto);
        for (const auto& m : onnx_metadata) {
            std::cout << m.first << ": " << m.second << std::endl;
        }
    }
}