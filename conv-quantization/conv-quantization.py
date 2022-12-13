import numpy as np

# int8截断，注意，-128丢掉了不要
def saturate(x):
    return np.clip(np.round(x), -127, +127)

def maxq(value):
    dynamic_range = np.abs(value).max()
    return dynamic_range / 127.0

def histogramq(value):
    hist, range = np.histogram(value, 100)
    total = len(value)

    left = 0
    right = len(hist)
    limit = 0.99
    while True:

        nleft = left + 1
        nright = right - 1
        left_cover = hist[nleft:right].sum() / total
        right_cover = hist[left:nright].sum() / total
        if left_cover < limit and right_cover < limit:
            break

        if left_cover > right_cover:
            left += 1
        else:
            right -= 1
    
    low  = range[left]
    high = range[right-1]
    dynamic_range = max(low, high)
    return dynamic_range / 127.0

class Q:
    def __init__(self, value):
        # 这里是对称量化，并不是非对称
        # 动态范围选取有多种方法，max/histogram/entropy等等
        self.scale = maxq(value)

    def __call__(self, f):
        return saturate(f / self.scale)

# x -> Q1 -> conv1 -> Q2 -> conv2 -> y
 
np.random.seed(31)
nelem   = 1000
x       = np.random.randn(nelem)
weight1 = np.random.randn(nelem)
bias1   = np.random.randn(nelem)
t       = x * weight1 + bias1
weight2 = np.random.randn(nelem)
bias2   = np.random.randn(nelem)
y       = t * weight2 + bias2

xQ      = Q(x)
w1Q     = Q(weight1)
tQ      = Q(t)
w2Q     = Q(weight2)

def QConv(x, w, b, iq, wq, oq=None):
    alpha     = iq.scale * wq.scale
    out_int32 = iq(x) * wq(w)

    if oq is None:
        # float output
        return out_int32 * alpha + b
    else:
        # int8 output
        return saturate((out_int32 * alpha + b) / oq.scale)

qt = QConv(x, weight1, bias1, xQ, w1Q, tQ)
y2 = QConv(qt, weight2, bias2, tQ, w2Q)

# print("Result t")
# print(t, "\n", qt * tQ.scale)
 
# print("\nResult y")
# print(y, "\n", y2)

y_diff = np.abs(y - y2).mean()
print(f"ydiff mean is: {y_diff}")