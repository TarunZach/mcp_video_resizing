__kernel void resize_bilinear(__global const uchar *src, int srcW, int srcH,
                              __global uchar *dst, int dstW, int dstH)
{
    int dx = get_global_id(0);
    int dy = get_global_id(1);

    if (dx >= dstW || dy >= dstH) return;

    float x_ratio = (float)(srcW - 1) / (dstW - 1);
    float y_ratio = (float)(srcH - 1) / (dstH - 1);
    float sx = x_ratio * dx;
    float sy = y_ratio * dy;
    int x = (int)sx;
    int y = (int)sy;
    float x_diff = sx - x;
    float y_diff = sy - y;

    int x1 = min(x + 1, srcW - 1);
    int y1 = min(y + 1, srcH - 1);

    for(int c = 0; c < 3; ++c) {
        int idx00 = (y  * srcW + x ) * 3 + c;
        int idx01 = (y  * srcW + x1) * 3 + c;
        int idx10 = (y1 * srcW + x ) * 3 + c;
        int idx11 = (y1 * srcW + x1) * 3 + c;

        float a = src[idx00];
        float b = src[idx01];
        float d = src[idx10];
        float e = src[idx11];

        float pixel = a*(1-x_diff)*(1-y_diff) + b*(x_diff)*(1-y_diff) +
                      d*(1-x_diff)*(y_diff) + e*(x_diff)*(y_diff);

        dst[(dy * dstW + dx)*3 + c] = (uchar)clamp(pixel, 0.0f, 255.0f);
    }
}


__kernel void bgr_to_yuv420(__global const uchar* bgr, int width, int height,
                            __global uchar* dstY,
                            __global uchar* dstU,
                            __global uchar* dstV)
{
    int x = get_global_id(0);
    int y = get_global_id(1);

    if(x >= width || y >= height) return;

    int idx = (y * width + x) * 3;
    float B = (float)bgr[idx + 0];
    float G = (float)bgr[idx + 1];
    float R = (float)bgr[idx + 2];

    float Y =  0.114f * B + 0.587f * G + 0.299f * R;
    dstY[y * width + x] = (uchar)clamp(Y, 0.0f, 255.0f);

    // For 2x2 pixel blocks, calculate and write U/V subsample once
    if ((x % 2 == 0) && (y % 2 == 0)) {
        float sumU = 0, sumV = 0;
        int samples = 0;
        for(int dy=0; dy<2; ++dy)
            for(int dx=0; dx<2; ++dx) {
                int sx = x + dx;
                int sy = y + dy;
                if(sx < width && sy < height) {
                    int sidx = (sy * width + sx) * 3;
                    float b = (float)bgr[sidx + 0];
                    float g = (float)bgr[sidx + 1];
                    float r = (float)bgr[sidx + 2];
                    float y =  0.114f * b + 0.587f * g + 0.299f * r;
                    float u = (b - y) * 0.565f + 128.0f;
                    float v = (r - y) * 0.713f + 128.0f;
                    sumU += u;
                    sumV += v;
                    samples++;
                }
            }
        int uv_idx = (y / 2) * (width / 2) + (x / 2);
        dstU[uv_idx] = (uchar)clamp(sumU / samples, 0.0f, 255.0f);
        dstV[uv_idx] = (uchar)clamp(sumV / samples, 0.0f, 255.0f);
    }
}
