#include "image_loader.h"
#include "opencv2/imgcodecs.hpp"
#include "opencv2/imgproc.hpp"
#include <GL/gl.h>
#include <filesystem>
#include <opencv2/core.hpp>
#include <opencv2/opencv.hpp>
#include <queue>
#include <string>


std::pair<ImageTexture*, cv::Mat*> ImageTexture::asyncTextureLoad(const char* src, int resizeDim, bool getDominantColors)
{
    std::cout << "-- Async Loading Texture\n";
    ImageTexture* s_img=new ImageTexture();
    cv::Mat* img  = new cv::Mat();
    *img = cv::imread(src, cv::IMREAD_COLOR);
    if (img->empty()) {
        s_img->isEmpty = true;
        std::cout << "IMAGE_LOADER_H::EmptyImage" << std::endl;
        std::cout << "IMAGE_LOADER_H::Loading Default Image" << std::endl;
        std::filesystem::remove(src);
        *img = cv::imread("./assets/images/default.jpg");
    }
    if (resizeDim > -1 && img->rows != resizeDim && !s_img->isEmpty) {
        cv::resize(*img, *img, cv::Size(resizeDim, resizeDim), cv::INTER_AREA);
        cv::imwrite(src, *img);
    }

    if (getDominantColors) {
        find_dominant_colors_result x = s_img->find_dominant_colors(*img, 5);
        s_img->dark[0] = x.colors[0][2];
        s_img->dark[1] = x.colors[0][1];
        s_img->dark[2] = x.colors[0][0];
        s_img->avg[0] = x.colors[2][2];
        s_img->avg[1] = x.colors[2][1];
        s_img->avg[2] = x.colors[2][0];
    }

    cv::cvtColor(*img, *img, cv::COLOR_BGR2RGBA);
    glGenTextures(1, &s_img->texture);
    s_img->image_width = img->cols;
    s_img->image_height = img->rows;
    std::cout << "-- Async Loading Done\n";
    return std::make_pair(s_img, img);
}

void ImageTexture::bindTexture(ImageTexture*& img, std::pair<ImageTexture*, cv::Mat*>& data)
{
    std::cout << "-- Binding Texture\n";
    img=data.first;
    glBindTexture(GL_TEXTURE_2D, img->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, data.second->cols, data.second->rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, data.second->data);

    // Freeing the resources
    glBindTexture(GL_TEXTURE_2D, 0);
    data.second->release();
    delete data.second;
    std::cout << "-- Binding Texture Done\n";
}


void ImageTexture::loadTextureFromFile(const std::string filename, int resizeDim, bool getDominantColors)
{
    cv::Mat img = cv::imread(filename, cv::IMREAD_COLOR);
    if (img.empty()) {
        isEmpty = true;
        std::cout << "IMAGE_LOADER_H::EmptyImage" << std::endl;
        std::cout << "IMAGE_LOADER_H::Loading Default Image" << std::endl;
        std::filesystem::remove(filename);
        img = cv::imread("./assets/images/default.jpg");
    }
    if (resizeDim > -1 && img.rows != resizeDim && !isEmpty) {
        cv::resize(img, img, cv::Size(resizeDim, resizeDim), cv::INTER_AREA);
        cv::imwrite(filename, img);
    }

    if (getDominantColors) {
        find_dominant_colors_result x = find_dominant_colors(img, 5);
        dark[0] = x.colors[0][2];
        dark[1] = x.colors[0][1];
        dark[2] = x.colors[0][0];
        avg[0] = x.colors[2][2];
        avg[1] = x.colors[2][1];
        avg[2] = x.colors[2][0];
    }

    cv::cvtColor(img, img, cv::COLOR_BGR2RGBA);
    glGenTextures(1, &this->texture);
    glBindTexture(GL_TEXTURE_2D, this->texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE); // This is required on WebGL for non power-of-two textures
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE); // Same
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, img.cols, img.rows, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.data);
    image_width = img.cols;
    image_height = img.rows;

    // Freeing the resources
    glBindTexture(GL_TEXTURE_2D, 0);
    img.release();
}

// void releaseMemory(){
//     img.release();
// }

find_dominant_colors_result ImageTexture::find_dominant_colors(cv::Mat& img, int count)
{
    find_dominant_colors_result result;
    const int width = img.cols;
    const int height = img.rows;

    cv::Mat classes = cv::Mat(height, width, CV_8UC1, cv::Scalar(1));
    t_color_node* root = new t_color_node();

    root->classid = 1;
    root->left = NULL;
    root->right = NULL;

    t_color_node* next = root;
    get_class_mean_cov(img, classes, root);
    for (int i = 0; i < count - 1; i++) {
        next = get_max_eigenvalue_node(root);
        partition_class(img, classes, get_next_classid(root), next);
        get_class_mean_cov(img, classes, next->left);
        get_class_mean_cov(img, classes, next->right);
    }

    result.colors = get_dominant_colors(root);

    cv::Mat dom = get_dominant_palette(result.colors);
    cv::imwrite("./palette.png", dom);
    return result;
}


std::vector<t_color_node*> ImageTexture::get_leaves(t_color_node* root)
{
    std::vector<t_color_node*> ret;
    std::queue<t_color_node*> queue;
    queue.push(root);

    while (queue.size() > 0) {
        t_color_node* current = queue.front();
        queue.pop();

        if (current->left && current->right) {
            queue.push(current->left);
            queue.push(current->right);
            continue;
        }

        ret.push_back(current);
    }

    return ret;
}

std::vector<cv::Vec3b> ImageTexture::get_dominant_colors(t_color_node* root)
{
    std::vector<t_color_node*> leaves = get_leaves(root);
    std::vector<cv::Vec3b> ret;

    for (size_t i = 0; i < leaves.size(); i++) {
        cv::Mat mean = leaves[i]->mean;
        ret.push_back(cv::Vec3b(mean.at<double>(0) * 255.0f, mean.at<double>(1) * 255.0f, mean.at<double>(2) * 255.0f));
    }

    return ret;
}

int ImageTexture::get_next_classid(t_color_node* root)
{
    int maxid = 0;
    std::queue<t_color_node*> queue;
    queue.push(root);

    while (queue.size() > 0) {
        t_color_node* current = queue.front();
        queue.pop();

        if (current->classid > maxid) maxid = current->classid;

        if (current->left != NULL) queue.push(current->left);

        if (current->right) queue.push(current->right);
    }

    return maxid + 1;
}

void ImageTexture::get_class_mean_cov(cv::Mat& img, cv::Mat& classes, t_color_node* node)
{
    const int width = img.cols;
    const int height = img.rows;
    const uchar classid = node->classid;

    cv::Mat mean = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
    cv::Mat cov = cv::Mat(3, 3, CV_64FC1, cv::Scalar(0));

    // We start out with the average color
    double pixcount = 0;
    for (int y = 0; y < height; y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        uchar* ptrClass = classes.ptr<uchar>(y);
        for (int x = 0; x < width; x++) {
            if (ptrClass[x] != classid) continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));
            scaled.at<double>(0) = color[0] / 255.0f;
            scaled.at<double>(1) = color[1] / 255.0f;
            scaled.at<double>(2) = color[2] / 255.0f;

            mean += scaled;
            cov = cov + (scaled * scaled.t());

            pixcount++;
        }
    }

    cov = cov - (mean * mean.t()) / pixcount;
    mean = mean / pixcount;

    // The node mean and covariance
    node->mean = mean.clone();
    node->cov = cov.clone();

    return;
}

void ImageTexture::partition_class(cv::Mat& img, cv::Mat& classes, uchar nextid, t_color_node* node)
{
    const int width = img.cols;
    const int height = img.rows;
    const int classid = node->classid;

    const uchar newidleft = nextid;
    const uchar newidright = nextid + 1;

    cv::Mat mean = node->mean;
    cv::Mat cov = node->cov;
    cv::Mat eigenvalues, eigenvectors;
    cv::eigen(cov, eigenvalues, eigenvectors);

    cv::Mat eig = eigenvectors.row(0);
    cv::Mat comparison_value = eig * mean;

    node->left = new t_color_node();
    node->right = new t_color_node();

    node->left->classid = newidleft;
    node->right->classid = newidright;

    // We start out with the average color
    for (int y = 0; y < height; y++) {
        cv::Vec3b* ptr = img.ptr<cv::Vec3b>(y);
        uchar* ptrClass = classes.ptr<uchar>(y);
        for (int x = 0; x < width; x++) {
            if (ptrClass[x] != classid) continue;

            cv::Vec3b color = ptr[x];
            cv::Mat scaled = cv::Mat(3, 1, CV_64FC1, cv::Scalar(0));

            scaled.at<double>(0) = color[0] / 255.0f;
            scaled.at<double>(1) = color[1] / 255.0f;
            scaled.at<double>(2) = color[2] / 255.0f;

            cv::Mat this_value = eig * scaled;

            if (this_value.at<double>(0, 0) <= comparison_value.at<double>(0, 0)) {
                ptrClass[x] = newidleft;
            } else {
                ptrClass[x] = newidright;
            }
        }
    }
    return;
}


t_color_node* ImageTexture::get_max_eigenvalue_node(t_color_node* current)
{
    double max_eigen = -1;
    cv::Mat eigenvalues, eigenvectors;

    std::queue<t_color_node*> queue;
    queue.push(current);

    t_color_node* ret = current;
    if (!current->left && !current->right) return current;

    while (queue.size() > 0) {
        t_color_node* node = queue.front();
        queue.pop();

        if (node->left && node->right) {
            queue.push(node->left);
            queue.push(node->right);
            continue;
        }

        cv::eigen(node->cov, eigenvalues, eigenvectors);
        double val = eigenvalues.at<double>(0);
        if (val > max_eigen) {
            max_eigen = val;
            ret = node;
        }
    }

    return ret;
}