#include "ros/ros.h"
#include "std_msgs/Float32MultiArray.h"
#include <iostream>
#include <fstream>
#include <vector>
#include <mutex>
#include <thread>
#include <signal.h>

class Logger {
public:
    Logger() : nh_(), loop_rate_(100) {
        // Initialize subscribers
        sensorRFT_sub_ = nh_.subscribe("sensorRFT", 10, &Logger::sensorRFTCallback, this);
        sensorData_sub_ = nh_.subscribe("sensor_data", 10, &Logger::sensorDataCallback, this);

        // Open CSV file for writing
        csv_file_.open("sensor_data_log.csv");
        if (!csv_file_.is_open()) {
            ROS_ERROR("Failed to open CSV file for writing.");
            ros::shutdown();
        }
        csv_file_ << "RFT_1,RFT_2,RFT_3,RFT_4,RFT_5,RFT_6,"
                  << "data_1,data_2,data_3,data_4" << std::endl;

        // Register signal handler for graceful shutdown
        signal(SIGINT, Logger::signalHandler);
    }

    ~Logger() {
        if (csv_file_.is_open()) {
            csv_file_.close();
        }
    }

    void spin() {
        while (ros::ok() && is_running_) {
            std::vector<float> sensorRFT_data = getSensorRFTData();
            std::vector<float> sensorData_data = getSensorData();

            if (csv_file_.is_open()) {
                // Write sensorRFT data to CSV
                for (size_t i = 0; i < sensorRFT_data.size(); ++i) {
                    csv_file_ << sensorRFT_data[i] << ",";
                }

                // Write only the first 4 values from sensorData to CSV !!!! Adjust Index
                for (size_t i = 4; i < 8; ++i) {
                    csv_file_ << sensorData_data[i];
                    if (i < 7) { // Add comma between values up to the 4th index
                        csv_file_ << ",";
                    }
                }
                csv_file_ << std::endl;
            }

            ros::spinOnce();
            loop_rate_.sleep();
        }
    }

private:
    ros::NodeHandle nh_;
    ros::Subscriber sensorRFT_sub_;
    ros::Subscriber sensorData_sub_;
    ros::Rate loop_rate_;
    std::ofstream csv_file_;
    static bool is_running_;
    static std::mutex data_mutex_;

    std::vector<float> sensorRFT_data_{0, 0, 0, 0, 0, 0};
    std::vector<float> sensorData_data_{0, 0, 0, 0, 0, 0, 0, 0};

    static void signalHandler(int signum) {
        ROS_INFO("Interrupt signal received. Saving CSV file and exiting...");
        is_running_ = false;
        ros::shutdown();
    }

    void sensorRFTCallback(const std_msgs::Float32MultiArray::ConstPtr& msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        sensorRFT_data_ = msg->data;
        ROS_INFO("Received sensorRFT data");
    }

    void sensorDataCallback(const std_msgs::Float32MultiArray::ConstPtr& msg) {
        std::lock_guard<std::mutex> lock(data_mutex_);
        sensorData_data_ = msg->data;
        ROS_INFO("Received sensor_data data");
    }

    std::vector<float> getSensorRFTData() {
        std::lock_guard<std::mutex> lock(data_mutex_);
        return sensorRFT_data_;
    }

    std::vector<float> getSensorData() {
        std::lock_guard<std::mutex> lock(data_mutex_);
        return sensorData_data_;
    }
};

// Initialize static member variables
bool Logger::is_running_ = true;
std::mutex Logger::data_mutex_;

int main(int argc, char **argv) {
    // Initialize the ROS node
    ros::init(argc, argv, "sensor_logger");
    Logger logger;
    logger.spin();
    return 0;
}
