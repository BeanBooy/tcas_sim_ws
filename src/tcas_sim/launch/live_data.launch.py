from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    ld = LaunchDescription()

    ############## ADSB ##############
    adsb_handler = Node(
            package='adsb_handler',
            executable='adsb_handler',
            output='screen',
            parameters=[{
               'host': '127.0.0.1',
               'port': '30003'
            }]
        )
    ld.add_action(adsb_handler)

    ############## STATIC PLANES ##############
    static_plane_count = 3

    for x in range(static_plane_count):
        plane = Node(
            package='tcas_sim',
            executable='static_plane',
            name=f's_plane_{x}',
            output='screen',
        )
        ld.add_action(plane)
    
    ############## MONITOR ##############
    monitor = Node(
            package='monitor',
            executable='monitor',
            output='screen',
        )
    ld.add_action(monitor)

    return ld




from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration


def launch_setup(context, *args, **kwargs):
    # read parameter during runtime
    s_planes_val = LaunchConfiguration('s_planes').perform(context)
    static_plane_count = int(s_planes_val)

    actions = []

    ############## STATIC PLANES ##############
    for x in range(static_plane_count):
        plane = Node(
            package='tcas_sim',
            executable='static_plane',
            name=f's_plane_{x}',
            output='screen',
        )
        actions.append(plane)

    ############## ADSB ##############
    adsb_handler = Node(
            package='adsb_handler',
            executable='adsb_handler',
            output='screen',
            parameters=[{
                'host': '127.0.0.1',
                'port': '30003'
            }]
        )
    actions.append(adsb_handler)

    return actions

def generate_launch_description():
    ld = LaunchDescription()

    arg = DeclareLaunchArgument(
        's_planes',
        default_value='3',
        description='Amount of static plane nodes.'
    )
    ld.add_action(arg)

    # create logic at runtime
    ld.add_action(OpaqueFunction(function=launch_setup))

    return ld