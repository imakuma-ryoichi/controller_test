# ROS2 Configuration

`comm_config.yaml` controls the receiver channel priority used by the
`joy_publisher` node.

- `priority`: ordered list of channels. Supported values are `wifi` and
  `bluetooth`.
- `stale_timeout_ms`: a channel is ignored when no packet arrives within this
  window.
- `poll_rate_hz`: receive polling rate for the ROS 2 node.
- `publish_on_new_data_only`: publish `/controller/joy` only when a selected
  channel received a new packet.
