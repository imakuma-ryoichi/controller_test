import os

from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, ExecuteProcess
from launch.substitutions import LaunchConfiguration


def generate_launch_description():
    pkg_dir = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
    executable_path = os.path.join(pkg_dir, 'build', 'joy_publisher')
    default_comm_config = os.path.join(pkg_dir, 'config', 'comm_config.yaml')
    default_receiver_config = os.path.abspath(
        os.path.join(
            pkg_dir,
            '..',
            'controller_receiver',
            'config',
            'receiver_connection.yaml'))

    return LaunchDescription([
        DeclareLaunchArgument(
            'comm_config_path',
            default_value=default_comm_config),
        DeclareLaunchArgument(
            'receiver_config_path',
            default_value=default_receiver_config),
        ExecuteProcess(
            cmd=[
                executable_path,
                '--ros-args',
                '-p',
                ['comm_config_path:=', LaunchConfiguration('comm_config_path')],
                '-p',
                [
                    'receiver_config_path:=',
                    LaunchConfiguration('receiver_config_path'),
                ],
            ],
            output='screen',
        ),
    ])
