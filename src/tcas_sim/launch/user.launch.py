from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction, ExecuteProcess
from launch.substitutions import LaunchConfiguration

def launch_setup(context, *args, **kwargs):
    # read parameter during runtime
    icao = LaunchConfiguration('icao').perform(context)

    actions = []

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
            'start_velocity': 150.0
        }]
    )
    actions.append(plane)

    ############## TELEOP (in seperat terminal) ##############
    teleop = ExecuteProcess(
        cmd=[
            'gnome-terminal', '--', 
            'ros2', 'run', 'tcas_sim', 'plane_teleop',
            '--ros-args', '-p', f'icao_hex:={icao}'
        ],
        output='screen'
    )
    actions.append(teleop)

    return actions


def generate_launch_description():
    ld = LaunchDescription()

    arg = DeclareLaunchArgument(
        'icao',
        default_value='ABCDEF',
        description='The ICAO-Hex code.'
    )
    ld.add_action(arg)

    # create logic at runtime
    ld.add_action(OpaqueFunction(function=launch_setup))

    return ld