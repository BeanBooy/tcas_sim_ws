from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess


def generate_launch_description():
    ld = LaunchDescription()

    icao = "A1b2C3"

    ############## USER PLANE ##############
    plane = Node(
            package='tcas_sim',
            executable='user_plane',
            name=f'u_plane',
            output='screen',
            parameters=[{
                'icao_hex': icao,
                'start_latitude': 48.6,
                'start_longitude': 11.2,
                'start_velocity': 200.0
            }]
        )
    ld.add_action(plane)

    ############## TELEOP (in seperat terminal) ##############
    teleop = ExecuteProcess(
        cmd=[
            'gnome-terminal', '--', 
            'ros2', 'run', 'tcas_sim', 'plane_teleop',
            '--ros-args', '-p', f'icao_hex:={icao}'
        ],
        output='screen'
    )
    ld.add_action(teleop)

    return ld