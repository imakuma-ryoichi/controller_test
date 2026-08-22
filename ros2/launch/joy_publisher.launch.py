from launch import LaunchDescription
from launch_ros.actions import Node
import os

def generate_launch_description():
    pkg_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    executable_path = os.path.join(pkg_dir, 'build', 'joy_publisher')

    return LaunchDescription([
        Node(
            package='',
            executable=executable_path,
            name='joy_publisher',
            output='screen',
        )
    ])
