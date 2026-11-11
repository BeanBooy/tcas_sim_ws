from launch import LaunchDescription
from launch_ros.actions import Node
from launch.actions import DeclareLaunchArgument, OpaqueFunction
from launch.substitutions import LaunchConfiguration


def launch_setup(context, *args, **kwargs):
    # read parameter during runtime
    s_planes_val = LaunchConfiguration('s_planes').perform(context)
    static_plane_count = int(s_planes_val)

    actions = []

    # static planes
    for x in range(static_plane_count):
        plane = Node(
            package='tcas_sim',
            executable='static_plane',
            name=f's_plane_{x}',
            output='screen',
        )
        actions.append(plane)
    
    # monitor
    monitor = Node(
        package='monitor',
        executable='monitor',
        output='screen',
    )
    actions.append(monitor)

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