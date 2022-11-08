
#include <builder/trt_builder.hpp>
#include <infer/trt_infer.hpp>
#include <common/ilogger.hpp>
#include <app/yolo.hpp>
#include <opencv2/opencv.hpp>
#include <algorithm>
#include "bytetrack/BYTETracker.h"
#include "deepsort/deepsort.hpp"
#include <stdio.h>

using namespace std;
using namespace cv;

static const char* cocolabels[] = {
    "person", "bicycle", "car", "motorcycle", "airplane",
    "bus", "train", "truck", "boat", "traffic light", "fire hydrant",
    "stop sign", "parking meter", "bench", "bird", "cat", "dog", "horse",
    "sheep", "cow", "elephant", "bear", "zebra", "giraffe", "backpack",
    "umbrella", "handbag", "tie", "suitcase", "frisbee", "skis",
    "snowboard", "sports ball", "kite", "baseball bat", "baseball glove",
    "skateboard", "surfboard", "tennis racket", "bottle", "wine glass",
    "cup", "fork", "knife", "spoon", "bowl", "banana", "apple", "sandwich",
    "orange", "broccoli", "carrot", "hot dog", "pizza", "donut", "cake",
    "chair", "couch", "potted plant", "bed", "dining table", "toilet", "tv",
    "laptop", "mouse", "remote", "keyboard", "cell phone", "microwave",
    "oven", "toaster", "sink", "refrigerator", "book", "clock", "vase",
    "scissors", "teddy bear", "hair drier", "toothbrush"
};

bool onnx_hub(const char* name, const char* save_to);

template<typename _Cond>
vector<Object> det2tracks(const ObjectDetector::BoxArray& array, const _Cond& cond){

    vector<Object> outputs;
    for(int i = 0; i < array.size(); ++i){
        auto& abox = array[i];

        if(!cond(abox)) continue;

        Object obox;
        obox.prob = abox.confidence;
        obox.label = abox.class_label;
        obox.rect[0] = abox.left;
        obox.rect[1] = abox.top;
        obox.rect[2] = abox.right - abox.left;
        obox.rect[3] = abox.bottom - abox.top;
        outputs.emplace_back(obox);
    }
    return outputs;
}

template<typename _Cond>
DeepSORT::BBoxes det2boxes(const ObjectDetector::BoxArray& array, const _Cond& cond){

    DeepSORT::BBoxes outputs;
    for(int i = 0; i < array.size(); ++i){
        auto& abox = array[i];

        if(!cond(abox)) continue;
        outputs.emplace_back(abox.left, abox.top, abox.right, abox.bottom);
    }
    return outputs;
}

static void inference_bytetrack(int deviceid, const string& engine_file, TRT::Mode mode, Yolo::Type type, const string& model_name){

    auto engine = Yolo::create_infer(
        engine_file,                // engine file
        type,                       // yolo type, Yolo::Type::V5 / Yolo::Type::X
        deviceid,                   // gpu id
        0.25f,                      // confidence threshold
        0.45f,                      // nms threshold
        Yolo::NMSMethod::FastGPU,   // NMS method, fast GPU / CPU
        1024,                       // max objects
        false                       // preprocess use multi stream
    );
    if(engine == nullptr){
        INFOE("Engine is nullptr");
        return;
    }

    VideoCapture cap("1652153992351250.mp4");
    auto fps = cap.get(cv::CAP_PROP_FPS);
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    BYTETracker tracker;
    cv::Mat image;
    tracker.config().set_initiate_state({
        0.1,  0.1,  0.1,  0.1,
        0.2,  0.2,  1,    0.2
    }).set_per_frame_motion({
        0.1,  0.1,  0.1,  0.1,
        0.2,  0.2,  1,    0.2
    }).set_max_time_lost(150);

    VideoWriter writer("output.mp4", cv::VideoWriter::fourcc('M', 'P', 'E', 'G'), fps, cv::Size(width, height));
    auto cond = [](const ObjectDetector::Box& b){return b.class_label == 0;};

    FILE* f = fopen("track.meta.txt", "wb");
    int t = 0;
    while(cap.read(image)){

        auto boxes = engine->commit(TRT::cvmat2image(image)).get();
        for(auto& box : boxes){
            if(box.class_label != 0) continue;

            fprintf(f, "%d %f %f %f %f %f\n", t, box.left, box.top, box.right, box.bottom, box.confidence);
        }
        t++;

        auto tracks = tracker.update(det2tracks(boxes, cond));
        for(auto& track : tracks){

            vector<float> tlwh = track.tlwh;
			bool vertical = tlwh[2] / tlwh[3] > 1.6;
			if (tlwh[2] * tlwh[3] > 20 && !vertical)
			{
				auto s = tracker.get_color(track.track_id);
                rectangle(image, Rect(tlwh[0], tlwh[1], tlwh[2], tlwh[3] * 0.3), Scalar(get<0>(s), get<1>(s), get<2>(s)), -1);

				putText(image, format("%d", track.track_id), Point(tlwh[0], tlwh[1] - 10), 
                        0, 2, Scalar(0, 0, 255), 3, LINE_AA);
                rectangle(image, Rect(tlwh[0], tlwh[1], tlwh[2], tlwh[3]), Scalar(get<0>(s), get<1>(s), get<2>(s)), 3);
			}
        }
        writer.write(image);
        printf("process.\n");
    }
    fclose(f);
    writer.release();
    printf("Done.\n");
}

static void inference_deepsort(int deviceid, const string& engine_file, TRT::Mode mode, Yolo::Type type, const string& model_name){

    auto engine = Yolo::create_infer(
        engine_file,                // engine file
        type,                       // yolo type, Yolo::Type::V5 / Yolo::Type::X
        deviceid,                   // gpu id
        0.25f,                      // confidence threshold
        0.45f,                      // nms threshold
        Yolo::NMSMethod::FastGPU,   // NMS method, fast GPU / CPU
        1024,                       // max objects
        false                       // preprocess use multi stream
    );

    auto extractor = TRT::load_infer("fastreid.trtmodel");
    if(engine == nullptr){
        INFOE("Engine is nullptr");
        return;
    }

    //VideoCapture cap("1652153992351250.mp4");
    VideoCapture cap("1652154022552518.mp4");
    auto fps = cap.get(cv::CAP_PROP_FPS);
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);

    auto config = DeepSORT::Config();
    cv::Mat image;
    config.set_initiate_state({
        0.3,  0.3,  0.5,  0.1,
        0.5,  0.5,  1,    0.2
    }).set_per_frame_motion({
        0.3,  0.3,  0.5,  0.5,
        0.5,  0.5,  1,    0.2
    });
    config.has_feature = true;
    config.distance_threshold = 0.1;
    config.nhit = 3;
    config.nbuckets = 30;
    config.max_age = 150;
    auto tracker = DeepSORT::create_tracker(config);

    VideoWriter writer("output.mp4", cv::VideoWriter::fourcc('M', 'P', 'E', 'G'), fps, cv::Size(width, height));
    auto cond = [](const ObjectDetector::Box& b){return b.class_label == 0;};

    FILE* f = fopen("track.meta.txt", "wb");
    int t = 0;
    while(cap.read(image)){
        // if(t < 100 || t >350){
        //     t++;
        //     continue;
        // }

        auto boxes = engine->commit(TRT::cvmat2image(image)).get();
        for(auto& box : boxes){
            if(box.class_label != 0) continue;

            fprintf(f, "%d %f %f %f %f %f\n", t, box.left, box.top, box.right, box.bottom, box.confidence);
        }
        t++;

        DeepSORT::BBoxes dbboxes;
        for(auto& box : boxes){
			if (box.class_label == 0)
			{
                auto roi = cv::Rect(cv::Point(box.left, box.top), cv::Point(box.right, box.bottom));
                roi = roi & cv::Rect(0, 0, image.cols, image.rows);
                cv::Mat crop = image(roi).clone();
                cv::resize(crop, crop, cv::Size(256, 256));
                crop.convertTo(crop, CV_32F);

                cv::Mat ms[3];
                auto inputtensor = extractor->input();
                auto outputtensor = extractor->output();
                printf("inputtensor.shape = %s\n", inputtensor->shape_string());

                for(int n = 0; n < 3; ++n){
                    ms[n] = cv::Mat(256, 256, CV_32F, inputtensor->cpu<float>(0, n));
                }
                cv::split(crop, ms);
                extractor->forward();

                cv::Mat ofeat(1, outputtensor->size(1), CV_32F, outputtensor->cpu<float>());
                cv::normalize(ofeat, ofeat, 1.0f, 0.0f, cv::NORM_L2);

                DeepSORT::Box dbox;
                dbox.left = box.left;
                dbox.top = box.top;
                dbox.right = box.right;
                dbox.bottom = box.bottom;
                dbox.feature = ofeat.clone();
                dbboxes.emplace_back(dbox);
			}
        }

        putText(image, format("%d", t), Point(10, 60), 0, 2, Scalar(0, 0, 255), 3, LINE_AA);
        auto tracks = tracker->update(dbboxes);
        DeepSORT::Box track_loc;
        bool has_obj = false;
        for(auto& track : tracks){

			if (track->is_confirmed() && track->time_since_update() == 0)
			{
				auto s = DeepSORT::get_color(track->id());
                auto loc = track->location();

                if(track->id() == 1){
                    has_obj = true;
                    track_loc = loc;
                }
				putText(image, format("%d", track->id()), Point(loc.left, loc.top+60), 
                        0, 2, Scalar(0, 0, 255), 3, LINE_AA);
                rectangle(image, Rect(loc.left, loc.top, loc.width(), loc.height()), Scalar(get<0>(s), get<1>(s), get<2>(s)), 3);
			}
        }

        if(has_obj){

            auto center = track_loc.center();
            int width = track_loc.width();
            int height = track_loc.height();

            int dsth = image.rows * 0.7;
            float scale = dsth / (float)height;
            cv::Mat T0 = cv::Mat::eye(3, 3, CV_32F);
            cv::Mat S = cv::Mat::eye(3, 3, CV_32F);
            cv::Mat T1 = cv::Mat::eye(3, 3, CV_32F);
            T0.at<float>(0, 2) = -center.x;
            T0.at<float>(1, 2) = -center.y;
            S.at<float>(0, 0) = scale;
            S.at<float>(1, 1) = scale;
            T1.at<float>(0, 2) = image.cols*0.5;
            T1.at<float>(1, 2) = image.rows*0.5;
            cv::Mat M = cv::Mat(T1 * S * T0)(cv::Range(0, 2), cv::Range(0, 3));
            cv::warpAffine(image, image, M, image.size(), 1, 0, cv::Scalar::all(128));
        }

        writer.write(image);
        printf("process.\n");
    }
    fclose(f);
    writer.release();
    printf("Done.\n");
}

static void test(Yolo::Type type, TRT::Mode mode, const string& model){

    int deviceid = 0;
    auto mode_name = TRT::mode_string(mode);
    TRT::set_device(deviceid);

    auto int8process = [=](int current, int count, const vector<string>& files, shared_ptr<TRT::Tensor>& tensor){

        INFO("Int8 %d / %d", current, count);

        for(int i = 0; i < files.size(); ++i){
            auto image = cv::imread(files[i]);
            Yolo::image_to_tensor(TRT::cvmat2image(image), tensor, type, i);
        }
    };

    const char* name = model.c_str();
    string onnx_file = iLogger::format("%s.onnx", name);
    INFO("===================== test %s %s %s ==================================", Yolo::type_name(type), mode_name, name);

    if(!onnx_hub(name, onnx_file.c_str()))
        return;

    string model_file = iLogger::format("%s.%s.trtmodel", name, mode_name);
    int test_batch_size = 16;
    
    if(!iLogger::exists(model_file)){
        TRT::compile(
            mode,                       // FP32、FP16、INT8
            test_batch_size,            // max batch size
            onnx_file,                  // source 
            model_file,                 // save to
            {},
            int8process,
            "inference"
        );
    }

    if(!iLogger::exists("fastreid.trtmodel")){
        TRT::compile(
            mode,                       // FP32、FP16、INT8
            test_batch_size,            // max batch size
            "fastreid.onnx",                  // source 
            "fastreid.trtmodel"
        );
    }

    inference_deepsort(deviceid, model_file, mode, type, name);
}

int main(){
    
    TRT::init_nv_plugins();
    test(Yolo::Type::V5, TRT::Mode::FP32, "yolov5s");
    //test(Yolo::Type::V3, TRT::Mode::FP32, "yolov3");
    return 0;
}