from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import ExecuteProcess


def generate_launch_description():
    ld = LaunchDescription()

    icao = "ABCDEF"

    ############## USER PLANE ##############
    plane = Node(
            package='tcas_sim',
            executable='user_plane',
            name=f'u_plane_99',
            output='screen',
            parameters=[{
                'icao_hex': icao
            }]
        )
    ld.add_action(plane)

    ############## TELEOP ##############
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