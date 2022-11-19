
#include <cuda_runtime.h>
#include <stdio.h>
#include <vector>
#include <string>
#include <algorithm>
#include <numeric>
#include <thrust/sort.h>
#include <thrust/unique.h>
#include <unordered_map>
#include <cooperative_groups.h>
#include <mma.h>
#include <stdlib.h>

using namespace std;

#define checkRuntime(call)  check_runtime(call, #call, __LINE__, __FILE__)

static bool inline check_runtime(cudaError_t e, const char* call, int line, const char *file){
    if (e != cudaSuccess) {
        fprintf(stderr, "CUDA Runtime error %s # %s, code = %s [ %d ] in file %s:%d\n", call, cudaGetErrorString(e), cudaGetErrorName(e), e, file, line);
        return false;
    }
    return true;
}

enum class MemoryType : int{
    None = 0,
    GPU  = 1,
    Managed = 2,
    Host = 3
};

template<typename T, MemoryType type=MemoryType::GPU>
class Memory{
public:
    T* ptr()   const{return ptr_;}
    size_t size() const{return size_;}
    size_t bytes() const{return size_ * sizeof(T);}
    MemoryType memtype() const{return type;}

    virtual ~Memory(){
        free_memory();
    }

    void alloc_or_resize_to(size_t size){
        if(capacity_ < size){
            free_memory();

            if constexpr(type == MemoryType::GPU)
                checkRuntime(cudaMalloc(&ptr_, size * sizeof(T)));
            else if constexpr(type == MemoryType::Managed)
                checkRuntime(cudaMallocManaged(&ptr_, size * sizeof(T)));
            else if constexpr(type == MemoryType::Host)
                checkRuntime(cudaMallocHost(&ptr_, size * sizeof(T)));
            capacity_ = size;
        }
        size_ = size;
    }

    void free_memory(){
        if(ptr_){
            if constexpr(type == MemoryType::GPU)
                checkRuntime(cudaFree(ptr_));
            else if constexpr(type == MemoryType::Managed)
                checkRuntime(cudaFree(ptr_));
            else if constexpr(type == MemoryType::Host)
                checkRuntime(cudaFreeHost(ptr_));

            ptr_     = nullptr;
            capacity_ = 0;
            size_     = 0;
        }
    }

    bool empty() const{ return ptr_ == nullptr;}

private:
    T* ptr_          = nullptr;
    size_t size_     = 0;
    size_t capacity_ = 0;
};

enum class DataType : int{
    None    = 0,
    Int32   = 1,
    Float16 = 2,
    Float32 = 3
};

struct Tensor{
    void* data = nullptr;
    bool owner = true;
    vector<int64_t> shape;
    DataType dtype;
    bool device = true;
};

static void free_tensor(Tensor* tensor){
    if(tensor){
        if(tensor->data && tensor->owner) {
            if(tensor->device){
                checkRuntime(cudaFree(tensor->data));
            }else{
                checkRuntime(cudaFreeHost(tensor->data));
            }
        }
        delete tensor;
    }
}

static Tensor* create_tensor(vector<int64_t> shape, DataType dtype){
    Tensor* output = new Tensor();
    output->owner = true;
    output->shape = shape;
    output->dtype = dtype;

    size_t volumn = std::accumulate(shape.begin(), shape.begin() + shape.size(), 1, std::multiplies<int>());
    size_t dtype_bytes_map[] = {0, 4, 2, 4};
    checkRuntime(cudaMalloc(&output->data, volumn * dtype_bytes_map[(int)dtype]));
    return output;
}

static Tensor* reference_tensor(void* data, vector<int64_t> shape, DataType dtype){

    Tensor* output = new Tensor();
    output->data = data;
    output->owner = false;
    output->shape = shape;
    output->dtype = dtype;
    return output;
}

static Tensor* load_tensor(const std::string& file, bool device=true){

    FILE* f = fopen(file.c_str(), "rb");
    if(f == nullptr) return nullptr;

    int head[3];
    fread(head, 1, sizeof(head), f);
    if(head[0] != 0x33ff1101){
        printf("This is invalid tensor file %s\n", file.c_str());
        fclose(f);
        return nullptr;
    }

    Tensor* output = new Tensor();
    output->owner = true;
    output->device = device;

    int ndim = head[1];
    int dtype = head[2];
    int dims[16];
    fread(dims, 1, ndim * sizeof(int), f);

    output->shape.resize(ndim);
    std::transform(dims, dims + ndim, output->shape.begin(), [](int x){return x;});

    int volumn = std::accumulate(dims, dims + ndim, 1, std::multiplies<int>());
    DataType dtype_map[] = {DataType::Float32, DataType::Float16, DataType::Int32};
    int dtype_bytes_map[]        = {4, 2, 4};
    size_t bytes = dtype_bytes_map[dtype] * volumn;
    vector<unsigned char> host_data(bytes);

    output->dtype = dtype_map[dtype];
    fread(host_data.data(), 1, bytes, f);
    fclose(f);

    if(device){
        checkRuntime(cudaMalloc(&output->data, bytes));
        checkRuntime(cudaMemcpy(output->data, host_data.data(), bytes, cudaMemcpyHostToDevice));
    }else{
        checkRuntime(cudaMallocHost(&output->data, bytes));
        checkRuntime(cudaMemcpy(output->data, host_data.data(), bytes, cudaMemcpyHostToHost));
    }
    checkRuntime(cudaDeviceSynchronize());
    return output;
}

static bool load_tensor_to(const std::string& file, void* to){

    FILE* f = fopen(file.c_str(), "rb");
    if(f == nullptr) return false;

    int head[3];
    fread(head, 1, sizeof(head), f);
    if(head[0] != 0x33ff1101){
        printf("This is invalid tensor file %s\n", file.c_str());
        fclose(f);
        return false;
    }

    int ndim = head[1];
    int dtype = head[2];
    int dims[16];
    fread(dims, 1, ndim * sizeof(int), f);

    int volumn = std::accumulate(dims, dims + ndim, 1, std::multiplies<int>());
    DataType dtype_map[] = {DataType::Float32, DataType::Float16, DataType::Int32};
    int dtype_bytes_map[]        = {4, 2, 4};
    size_t bytes = dtype_bytes_map[dtype] * volumn;
    vector<unsigned char> host_data(bytes);

    fread(host_data.data(), 1, bytes, f);
    fclose(f);

    checkRuntime(cudaMemcpy(to, host_data.data(), bytes, cudaMemcpyHostToDevice));
    checkRuntime(cudaDeviceSynchronize());
    return true;
}

static bool save_tensor(const Tensor* tensor, const std::string& file, cudaStream_t stream){

    FILE* f = fopen(file.c_str(), "wb");
    if(f == nullptr){
        printf("Failed to open %s\n", file.c_str());
        return false;
    }

    std::unordered_map<DataType, int> dtype_map{
        {DataType::Float32, 0},
        {DataType::Float16, 1},
        {DataType::Int32, 2}
    };

    std::unordered_map<DataType, int> sizeof_dtype{
        {DataType::Float32, 4},
        {DataType::Float16, 2},
        {DataType::Int32, 4}
    };

    int head[] = {0x33ff1101, (int)tensor->shape.size(), dtype_map[tensor->dtype]};
    int dims[16];
    int i = 0;
    size_t bytes = 1;
    for(auto dim : tensor->shape){
        dims[i++] = dim;
        bytes *= dim;
    }
    bytes *= sizeof_dtype[tensor->dtype];

    std::vector<char> host_data(bytes);
    checkRuntime(cudaMemcpyAsync(host_data.data(), tensor->data, bytes, cudaMemcpyDeviceToHost, stream));
    checkRuntime(cudaStreamSynchronize(stream));

    fwrite(head, 1, sizeof(head), f);
    fwrite(dims, 1, tensor->shape.size() * sizeof(int), f);
    fwrite(host_data.data(), 1, bytes, f);
    fclose(f);
    return true;
}

#define M    320
#define N    320
#define K    320
#define mtile    16
#define ntile     8
#define ktile     8
#define MBLOCK   32
#define NBLOCK   32
#define KBLOCK   32

__device__ void zload_global_to_shared_memory_16byte(void* dst, const void* src, int real_size){

    unsigned int shmem_ptr = __cvta_generic_to_shared(dst);
    asm volatile("cp.async.cg.shared.global [%0], [%1], 16, %2;" ::"r"(shmem_ptr), "l"(src), "r"(real_size));
}

__global__ void gemm(half* a, half* b, half* c){

    __shared__ half shared_a[MBLOCK][KBLOCK];
    __shared__ half shared_b[KBLOCK][NBLOCK];
    int ithread = threadIdx.x;
    int mblock  = blockIdx.x * MBLOCK;
    int nblock  = blockIdx.y * NBLOCK;
    int accumulator[2][4][2] = {0};

    for(int kblock = 0; kblock < K; kblock += KBLOCK){
        for(int iwarp = 0; iwarp < 4; ++iwarp){
            /*每个warp A = 2个16x8    B = 4个8x8，分别排列为一行*/
            zload_global_to_shared_memory_16byte(
                &shared_a[ithread][iwarp * 8], a + (mblock + ithread) * K + kblock + iwarp * 8, 16
            );
    
            zload_global_to_shared_memory_16byte(
                &shared_b[ithread][iwarp * 8], b + (nblock + ithread) * K + kblock + iwarp * 8, 16
            );
    
            asm volatile ("cp.async.wait_all;");
            __syncthreads();

            unsigned int shmem_ptra = __cvta_generic_to_shared(&shared_a[ithread][iwarp * 8]);
            unsigned int shmem_ptrb = __cvta_generic_to_shared(&shared_b[ithread][iwarp * 8]);
    
            uint4 val_a, val_b;
            asm volatile ("ldmatrix.sync.aligned.x4.m8n8.shared.b16 {%0, %1, %2, %3}, [%4];" : "=r"(val_a.x), "=r"(val_a.y), "=r"(val_a.z), "=r"(val_a.w) : "r"(shmem_ptra));
            asm volatile ("ldmatrix.sync.aligned.x4.m8n8.shared.b16 {%0, %1, %2, %3}, [%4];" : "=r"(val_b.x), "=r"(val_b.y), "=r"(val_b.z), "=r"(val_b.w) : "r"(shmem_ptrb));

            for(int i = 0; i < 2; ++i){
                for(int j = 0; j < 4; ++j){
                    asm volatile(
                        "mma.sync.aligned.m16n8k8.row.col.f16.f16.f16.f16 {%0,%1}, {%2,%3}, {%4}, {%5,%6};"
                        : "=r"(accumulator[i][j][0]), "=r"(accumulator[i][j][1])
                        : "r"(((uint2*)&val_a)[i].x), "r"(((uint2*)&val_a)[i].y),
                        "r"(((uint32_t*)&val_b)[j]),
                        "r"(accumulator[i][j][0]), "r"(accumulator[i][j][1])
                    );
                }
            }
        }
    }

    for(int i = 0; i < 2; ++i){
        for(int j = 0; j < 4; ++j){
            *(uint32_t*)&c[((mblock + i * 16 + ithread / 4) * N + nblock + j * 8 + (ithread % 4) * 2)]     = accumulator[i][j][0];
            *(uint32_t*)&c[((mblock + i * 16 + ithread / 4 + 8) * N + nblock + j * 8 + (ithread % 4) * 2)] = accumulator[i][j][1];
        }
    }
}

void __global__ arange(half* p, int n){
    int idx = threadIdx.x + blockDim.x * blockIdx.x;
    if(idx >= n) return;

    uint32_t randval = idx ^ 0xFFAAC031;
    randval <<= 8;
    randval ^= (uint32_t)&randval;
    randval <<= 5;
    randval ^= 0x3192FF01;
    randval = randval % 100;
    int sign = randval % 2 == 0 ? -1 : 1;
    p[idx] = sign * (int)randval / 1000.0f; 
}

int main(){

    Memory<half> A, B, C;
    A.alloc_or_resize_to(M * K);
    B.alloc_or_resize_to(K * N);
    C.alloc_or_resize_to(M * N);

    cudaStream_t stream;
    cudaStreamCreate(&stream);

    arange<<<(M * K + 1023) / 1024, 1024, 0, stream>>>(A.ptr(), M * K);
    arange<<<(K * N + 1023) / 1024, 1024, 0, stream>>>(B.ptr(), K * N);
    arange<<<(M * N + 1023) / 1024, 1024, 0, stream>>>(C.ptr(), M * N);

    dim3 block(32, 1);
    dim3 grid((M + MBLOCK - 1) / MBLOCK, (N + NBLOCK - 1) / NBLOCK);
    gemm<<<grid, block, 0, stream>>>(A.ptr(), B.ptr(), C.ptr());
    cudaStreamSynchronize(stream);

    save_tensor(reference_tensor(A.ptr(), {M, K}, DataType::Float16), "A.bin", stream);
    save_tensor(reference_tensor(B.ptr(), {K, N}, DataType::Float16), "B.bin", stream);
    save_tensor(reference_tensor(C.ptr(), {M, N}, DataType::Float16), "C.bin", stream);
    return 0;
}