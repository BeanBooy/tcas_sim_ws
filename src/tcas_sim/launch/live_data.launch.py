from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    ld = LaunchDescription()

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

    scenario_planes = [
        {'id': 'Flight_A', 'lat': 10.0, 'lon': 48.3},
        {'id': 'Flight_B', 'lat': 10.1, 'lon': 48.4},
        {'id': 'Flight_C', 'lat': 10.2, 'lon': 48.2},
    ]

    counter = 0
    for plane in scenario_planes:
        plane = Node(
            package='tcas_sim',
            executable='plane_node',
            name=f'plane_{counter}',
            output='screen',
            parameters=[{
                'id': plane['id']
            }]
        )
        ld.add_action(plane)
        counter += 1

    return ld