from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    ld = LaunchDescription()
    
    ############## MONITOR ##############
    monitor = Node(
            package='monitor',
            executable='monitor',
            output='screen',
        )
    ld.add_action(monitor)

    return ld