import rclpy
from rclpy.node import Node
import pandas as pd
from sensor_msgs.msg import PointCloud2, PointField
import sensor_msgs_py.point_cloud2 as pc2
from std_msgs.msg import Header
import os

class PointCloudPublisher(Node):
    def __init__(self):
        super().__init__('pointcloud_publisher')
        self.publisher_ = self.create_publisher(PointCloud2, 'pointcloud', 10)
        self.timer = self.create_timer(0.1, self.timer_callback)  # 10 Hz

        self.folder_path = '/root/csv_lidar'
        self.csv_files = sorted(os.listdir(self.folder_path))
        self.current_file_index = 0
        self.total_files = len(self.csv_files)

    def timer_callback(self):
        if self.current_file_index >= self.total_files:
            self.current_file_index = 0  # Loop back to the start

        csv_file = os.path.join(self.folder_path, self.csv_files[self.current_file_index])
        self.get_logger().info(f"Reading file: {csv_file}")
        
        try:
            frame_data = pd.read_csv(csv_file, encoding='latin1', index_col=0)
            if frame_data.empty:
                self.get_logger().error(f"DataFrame is empty. File: {csv_file}")
                self.current_file_index += 1
                return

            self.get_logger().info(f"DataFrame columns: {frame_data.columns.tolist()}")
            self.get_logger().info(f"First few rows:\n{frame_data.head()}")

            points = []
            for _, row in frame_data.iterrows():
                point = [row['x']*0.01, row['y']*0.01, row['z']*0.01, row['Reflectivity']]
                points.append(point)

            header = Header()
            header.stamp = self.get_clock().now().to_msg()
            header.frame_id = 'map'

            fields = [
                PointField(name='x', offset=0, datatype=PointField.FLOAT32, count=1),
                PointField(name='y', offset=4, datatype=PointField.FLOAT32, count=1),
                PointField(name='z', offset=8, datatype=PointField.FLOAT32, count=1),
                PointField(name='intensity', offset=12, datatype=PointField.FLOAT32, count=1)
            ]

            cloud = pc2.create_cloud(header, fields, points)
            self.publisher_.publish(cloud)
            self.current_file_index += 1
        except FileNotFoundError:
            self.get_logger().error(f"File not found: {csv_file}")
        except pd.errors.EmptyDataError:
            self.get_logger().error(f"No data: {csv_file}")
        except KeyError as e:
            self.get_logger().error(f"KeyError: {e}. Available columns: {frame_data.columns.tolist()}")
        except Exception as e:
            self.get_logger().error(f"Unexpected error: {e}")

def main(args=None):
    rclpy.init(args=args)
    node = PointCloudPublisher()
    rclpy.spin(node)
    node.destroy_node()
    rclpy.shutdown()

if __name__ == '__main__':
    main()
