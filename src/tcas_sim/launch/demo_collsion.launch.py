from launch import LaunchDescription
from launch.actions import ExecuteProcess

def generate_launch_description():
    ld = LaunchDescription()

    plane_alpha = ExecuteProcess(
        cmd=[
            'gnome-terminal', '--',
            'ros2', 'run', 'tcas_sim', 'user_plane',
            '--ros-args',
            '-p', 'icao_hex:=ABC123',
            '-p', 'start_latitude:=48.3000',
            '-p', 'start_longitude:=11.4300',
            '-p', 'start_altitude:=1000.0',
            '-p', 'start_heading:=0.0',
            '-p', 'start_velocity:=190.0'
        ],
        output='screen'
    )
    ld.add_action(plane_alpha)

    teleop_alpha = ExecuteProcess(
        cmd=[
            'gnome-terminal', '--',
            'ros2', 'run', 'tcas_sim', 'plane_teleop',
            '--ros-args', '-p', 'icao_hex:=ABC123'
        ],
        output='screen'
    )
    ld.add_action(teleop_alpha)


    plane_bravo = ExecuteProcess(
        cmd=[
            'gnome-terminal', '--',
            'ros2', 'run', 'tcas_sim', 'user_plane',
            '--ros-args',
            '-p', 'icao_hex:=DEF456',
            '-p', 'start_latitude:=48.3700',
            '-p', 'start_longitude:=11.5354',
            '-p', 'start_altitude:=1000.0',
            '-p', 'start_heading:=270.0',
            '-p', 'start_velocity:=190.0'
        ],
        output='screen'
    )
    ld.add_action(plane_bravo)

    teleop_bravo = ExecuteProcess(
        cmd=[
            'gnome-terminal', '--',
            'ros2', 'run', 'tcas_sim', 'plane_teleop',
            '--ros-args', '-p', 'icao_hex:=DEF456'
        ],
        output='screen'
    )
    ld.add_action(teleop_bravo)

    return ld
