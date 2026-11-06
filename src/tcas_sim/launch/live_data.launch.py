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

    ############## USER PLANES ##############
    # i dont think user planes should be launched via this file...
    # each user plane should be startet manually (or with a seperat launch file) on the control machine
    # plane = Node(
    #         package='tcas_sim',
    #         executable='user_plane',
    #         name=f'u_plane_99',
    #         output='screen',
    #     )
    # ld.add_action(plane)

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