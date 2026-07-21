"""Launch semantic_seg with the PP-LiteSeg parameter defaults."""
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    params_file = PathJoinSubstitution(
        [FindPackageShare("semantic_seg"), "config", "semantic_seg.yaml"]
    )
    return LaunchDescription(
        [
            DeclareLaunchArgument("use_camera", default_value="true"),
            DeclareLaunchArgument(
                "image_topic", default_value="/camera/image_raw"
            ),
            Node(
                package="semantic_seg",
                executable="semantic_seg_node",
                name="semantic_seg_node",
                output="screen",
                parameters=[
                    params_file,
                    {
                        "use_camera": LaunchConfiguration("use_camera"),
                        "image_topic": LaunchConfiguration("image_topic"),
                    },
                ],
            ),
        ]
    )
