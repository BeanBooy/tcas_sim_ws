from launch import LaunchDescription
from launch.actions import ExecuteProcess, LogInfo, RegisterEventHandler, TimerAction
from launch.event_handlers import OnProcessExit
from launch_ros.actions import Node


def make_intruder(latitude, altitude):
    pose = (
        '{icao_hex: B2C3D4, '
        f'latitude: {latitude}, longitude: 11.0, altitude: {altitude}, '
        'heading: 180.0, velocity: 50.0, vertical_speed: 0.0, '
        'pitch: 0.0, roll: 0.0}'
    )
    return ExecuteProcess(
        cmd=[
            'ros2', 'topic', 'pub', '--times', '15', '--rate', '10', '--print', '0',
            '--wait-matching-subscriptions', '1',
            '--max-wait-time-secs', '10',
            '/airspace_broadcast', 'interfaces/msg/Pose', pose,
        ],
        output='screen',
    )


def generate_launch_description():

    own_plane = Node(
        package='tcas_sim',
        executable='user_plane',
        name='tcas_test_ownship',
        output='screen',
        parameters=[{
            'icao_hex': 'A1B2C3',
            'start_latitude': 48.0,
            'start_longitude': 11.0,
            'start_altitude': 1000.0,
            'start_heading': 0.0,
        }],
    )

    ta = make_intruder(48.025, 1000.0)
    ra = make_intruder(48.015, 1000.0)
    clear = make_intruder(48.015, 2000.0)

    return LaunchDescription([
        own_plane,
        # Trigger the next step only when the previous ros2 topic pub exits.
        RegisterEventHandler(OnProcessExit(
            target_action=ta,
            on_exit=[TimerAction(period=0.2, actions=[
                LogInfo(msg='TEST 2/3: RA -- closer intruder'),
                ra,
            ])],
        )),
        RegisterEventHandler(OnProcessExit(
            target_action=ra,
            on_exit=[TimerAction(period=0.2, actions=[
                LogInfo(msg='TEST 3/3: CLEAR -- intruder 1000 m higher'),
                clear,
            ])],
        )),
        RegisterEventHandler(OnProcessExit(
            target_action=clear,
            on_exit=[LogInfo(msg='TCAS test complete. Press Ctrl+C to stop.')],
        )),
        TimerAction(period=3.0, actions=[
            LogInfo(msg='TEST 1/3: TA -- approaching intruder'),
            ta,
        ]),
    ])
