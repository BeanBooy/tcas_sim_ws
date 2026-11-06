from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    ld = LaunchDescription()

    ############## USER PLANE ##############
    plane = Node(
            package='tcas_sim',
            executable='user_plane',
            name=f'u_plane_99',
            output='screen',
            parameters=[{
                'flight_number': 'USER'
            }]
        )
    ld.add_action(plane)
    
    ############## TELEOP ##############
    teleop = Node(
            package='tcas_sim',
            executable='plane_teleop',
            output='screen',
        )
    ld.add_action(teleop)

    return ld