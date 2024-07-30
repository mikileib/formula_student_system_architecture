#include <rclcpp/rclcpp.hpp>
#include <sensor_msgs/msg/point_cloud2.hpp>
#include <sensor_msgs/point_cloud2_iterator.hpp>
#include <fstream>
#include <vector>
#include <string>
#include <filesystem>
#include <chrono>

class CsvToPointCloud2Node : public rclcpp::Node
{
public:
    CsvToPointCloud2Node() : Node("csv_to_pointcloud2_converter")
    {
        this->declare_parameter("csv_folder", "/root/csv_lidar");
        csv_folder_ = this->get_parameter("csv_folder").as_string();

        publisher_ = this->create_publisher<sensor_msgs::msg::PointCloud2>("pointcloud", 10);
        timer_ = this->create_wall_timer(std::chrono::milliseconds(100), std::bind(&CsvToPointCloud2Node::timer_callback, this));
        
        RCLCPP_INFO(this->get_logger(), "Reading CSV files from: %s", csv_folder_.c_str());
        for (const auto & entry : std::filesystem::directory_iterator(csv_folder_))
        {
            if (entry.path().extension() == ".csv")
            {
                csv_files_.push_back(entry.path().string());
            }
        }
        std::sort(csv_files_.begin(), csv_files_.end());
        RCLCPP_INFO(this->get_logger(), "Found %zu CSV files", csv_files_.size());
    }
private:
    void timer_callback()
    {
        if (current_file_index_ >= csv_files_.size())
        {
            current_file_index_ = 0;
        }

        auto points = read_csv(csv_files_[current_file_index_]);
        auto cloud_msg = create_cloud_msg(points);
        publisher_->publish(*cloud_msg);

        current_file_index_++;
    }

    std::vector<std::array<float, 4>> read_csv(const std::string& file_path)
    {
        std::vector<std::array<float, 4>> points;
        std::ifstream file(file_path);
        std::string line;
        
        // Skip header
        std::getline(file, line);

        while (std::getline(file, line))
        {
            std::istringstream iss(line);
            std::string token;
            std::vector<float> row;

            for (int i = 0; i < 10 && std::getline(iss, token, ','); ++i)
            {
                if (i == 5 || i == 6 || i == 7 || i == 8 || i == 9)
                {
                    row.push_back(std::stof(token));
                }
            }

            if (row.size() == 5 && row[1] <= 0.01f) // Check 'Probability of False Alarm'
            {
                points.push_back({row[2], row[3], row[4], row[0]}); // x, y, z, intensity
            }
        }

        return points;
    }

    std::unique_ptr<sensor_msgs::msg::PointCloud2> create_cloud_msg(const std::vector<std::array<float, 4>>& points)
    {
        auto cloud_msg = std::make_unique<sensor_msgs::msg::PointCloud2>();
        cloud_msg->header.stamp = this->now();
        cloud_msg->header.frame_id = "map";
        cloud_msg->height = 1;
        cloud_msg->width = points.size();
        cloud_msg->is_dense = true;
        cloud_msg->is_bigendian = false;
        cloud_msg->point_step = 4 * sizeof(float);
        cloud_msg->row_step = cloud_msg->point_step * cloud_msg->width;

        sensor_msgs::PointCloud2Modifier modifier(*cloud_msg);
        modifier.setPointCloud2Fields(4,
            "x", 1, sensor_msgs::msg::PointField::FLOAT32,
            "y", 1, sensor_msgs::msg::PointField::FLOAT32,
            "z", 1, sensor_msgs::msg::PointField::FLOAT32,
            "intensity", 1, sensor_msgs::msg::PointField::FLOAT32);
        
        modifier.resize(points.size());

        sensor_msgs::PointCloud2Iterator<float> iter_x(*cloud_msg, "x");
        sensor_msgs::PointCloud2Iterator<float> iter_y(*cloud_msg, "y");
        sensor_msgs::PointCloud2Iterator<float> iter_z(*cloud_msg, "z");
        sensor_msgs::PointCloud2Iterator<float> iter_intensity(*cloud_msg, "intensity");

        for (const auto& point : points)
        {
            *iter_x = point[0]*0.01;
            *iter_y = point[1]*0.01;
            *iter_z = point[2]*0.01;
            *iter_intensity = point[3];
            ++iter_x; ++iter_y; ++iter_z; ++iter_intensity;
        }

        return cloud_msg;
    }

    rclcpp::Publisher<sensor_msgs::msg::PointCloud2>::SharedPtr publisher_;
    rclcpp::TimerBase::SharedPtr timer_;
    std::string csv_folder_;
    std::vector<std::string> csv_files_;
    size_t current_file_index_ = 0;
};


int main(int argc, char* argv[])
{
    rclcpp::init(argc, argv);
    auto node = std::make_shared<CsvToPointCloud2Node>();
    rclcpp::spin(node);
    rclcpp::shutdown();
    return 0;
}
