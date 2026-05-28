DWIN DGUS Display
=================

.. seo::
    :description: Instructions for setting up DWIN DGUS/T5L UART displays in ESPHome.
    :image: dwin.svg

The ``dwin`` display platform supports DWIN DGUS/T5L HMI panels over UART.
DWIN panels are project-driven displays: the screen layout is authored in the
DWIN DGUS tooling, and ESPHome exchanges values with the panel by reading and
writing Variable Pointer (VP) addresses.

Protocol overview
-----------------

The DGUS serial frame used by this component is::

    5A A5 LEN CMD PAYLOAD...

``LEN`` is the number of bytes after the length byte, including ``CMD``.
The commonly used DGUS commands are:

- ``0x80``: write register
- ``0x81``: read register
- ``0x82``: write VP
- ``0x83``: read VP / VP data returned by the panel

VP addresses and payload words are big-endian. A VP read request uses command
``0x83`` with payload ``VP_H VP_L WORD_COUNT``. A VP data response uses command
``0x83`` with payload ``VP_H VP_L WORD_COUNT DATA...``.

.. note::

    DWIN projects vary. Confirm your VP map from your DGUS project files. The
    defaults below match common DGUS II conventions, but project-specific system
    variable mapping can differ.

Configuration variables
-----------------------

- **id** (**Required**, :ref:`config-id`): The display id.
- **uart_id** (*Optional*, :ref:`config-id`): UART bus to use.
- **brightness** (*Optional*, percentage): Initial brightness.
- **brightness_address** (*Optional*, hex uint16, default ``0x0082``): VP used for brightness writes.
- **page_address** (*Optional*, hex uint16, default ``0x0084``): VP used for page switching.
- **command_spacing** (*Optional*, :ref:`config-time`, default ``0ms``): Delay between queued UART commands.
- **max_queue_size** (*Optional*, int, default ``32``): Maximum queued command count.
- **lambda** (*Optional*, :ref:`lambda <config-lambda>`): Periodic display writer.
- **on_vp_data** (*Optional*, :ref:`Automation <automation>`): Called when VP data is received.
- **on_register_data** (*Optional*, :ref:`Automation <automation>`): Called when register data is received.
- **on_buffer_overflow** (*Optional*, :ref:`Automation <automation>`): Called when the transmit queue overflows.

Example
-------

.. code-block:: yaml

    uart:
      id: dwin_uart
      tx_pin: GPIO17
      rx_pin: GPIO16
      baud_rate: 115200

    display:
      - platform: dwin
        id: panel
        update_interval: 1s
        command_spacing: 10ms
        lambda: |-
          it.set_text(0x1000, "ESPHome", 16);
          it.set_word(0x1100, 42);
        on_vp_data:
          then:
            - lambda: |-
                ESP_LOGI("dwin", "VP 0x%04X words=%u", vp, data.size());

Actions
-------

``display.dwin.write_word``
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Writes one 16-bit word to a VP address using DGUS command ``0x82``.

.. code-block:: yaml

    on_...:
      - display.dwin.write_word:
          id: panel
          address: 0x1100
          value: 123

``display.dwin.write_text``
~~~~~~~~~~~~~~~~~~~~~~~~~~~

Writes a fixed-length, zero-padded byte string to a VP address.

.. code-block:: yaml

    on_...:
      - display.dwin.write_text:
          id: panel
          address: 0x1000
          text: "Hello"
          length: 16

``display.dwin.read_vp``
~~~~~~~~~~~~~~~~~~~~~~~~

Requests one or more words from a VP address using DGUS command ``0x83``.

.. code-block:: yaml

    on_...:
      - display.dwin.read_vp:
          id: panel
          address: 0x2000
          words: 2

``display.dwin.set_page``
~~~~~~~~~~~~~~~~~~~~~~~~~

Switches page using the common DGUS system VP payload ``5A 01 00 PP`` written to
``page_address``.

.. code-block:: yaml

    on_...:
      - display.dwin.set_page:
          id: panel
          page: 1

``display.dwin.set_brightness``
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Writes a 0-100 percentage value to ``brightness_address``.

.. code-block:: yaml

    on_...:
      - display.dwin.set_brightness:
          id: panel
          brightness: 50%

C++ lambda helpers
------------------

The display object exposes low-level helpers for project-specific VP maps:

.. code-block:: cpp

    it.set_word(0x1100, 42);
    it.set_text(0x1000, "Hello", 16);
    it.request_words(0x2000, 2);
    it.set_page(1);
    it.set_brightness(0.5f);

Upstreaming status
------------------

This component is an initial upstream-oriented scaffold. Before merge it still
needs hardware validation across at least one DGUS II/T5L panel family, more
examples for common VP map patterns, and docs screenshots/diagrams if accepted
by ESPHome maintainers.
