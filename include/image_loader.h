#ifndef IMG_LOAD_H
#define IMG_LOAD_H

#define STB_IMAGE_IMPLEMENTATION
#include <filesystem>
#include <gl/gl.h>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <queue>
#include <string>
#define GL_CLAMP_TO_EDGE 0x812F
#define GL_CLAMP 0x2900
#define GL_CLAMP_TO_BORDER 0x812D


struct find_dominant_colors_result {
    std::vector<cv::Vec3b> colors;
};


typedef struct t_color_node {
    cv::Mat mean; // The mean of this node
    cv::Mat cov;
    uchar classid; // The class ID

    t_color_node* left;
    t_color_node* right;
} t_color_node;


struct ImageTexture {
    int image_height = 0;
    int image_width = 0;
    bool isEmpty = false;
    GLuint texture;
    std::array<int, 3> avg;
    std::array<int, 3> dark;
    bool textureLoaded = false;

    static std::pair<ImageTexture*, cv::Mat*> asyncTextureLoad(const char* src, int resizeDim = -1, bool getDominantColors = false);
    static void bindTexture(ImageTexture*& img, std::pair<ImageTexture*, cv::Mat*>& data);

    void loadTextureFromFile(const std::string filename, int resizeDim = -1, bool getDominantColors = false);

    find_dominant_colors_result find_dominant_colors(cv::Mat& img, int count);

    std::vector<t_color_node*> get_leaves(t_color_node*);

    std::vector<cv::Vec3b> get_dominant_colors(t_color_node*);

    int get_next_classid(t_color_node*);

    void get_class_mean_cov(cv::Mat&, cv::Mat&, t_color_node*);

    void partition_class(cv::Mat& img, cv::Mat& classes, uchar, t_color_node*);

    t_color_node* get_max_eigenvalue_node(t_color_node*);

    cv::Mat get_dominant_palette(std::vector<cv::Vec3b> colors)
    {
        const int tile_size = 64;
        cv::Mat ret = cv::Mat(tile_size, tile_size * colors.size(), CV_8UC3, cv::Scalar(0));
        for (size_t i = 0; i < colors.size(); i++) {
            cv::Rect rect(i * tile_size, 0, tile_size, tile_size);
            cv::rectangle(ret, rect, cv::Scalar(colors[i][0], colors[i][1], colors[i][2]), cv::FILLED);
        }
        return ret;
    }
};


#endif
