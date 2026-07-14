"""Launch semantic_seg with the PP-LiteSeg parameter defaults."""
from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    return LaunchDescription([Node(
        package="semantic_seg", executable="semantic_seg_node", name="semantic_seg_node",
        output="screen", parameters=[PathJoinSubstitution(
            [FindPackageShare("semantic_seg"), "config", "semantic_seg.yaml"])]
    )])
