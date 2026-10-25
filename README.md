###### _<div align="right"></a><sub>simple, not required to exceed 15,000 lines of code</sub></div>_

<p align="center">
  <img src="images/roxip-banner.png" alt="roxip" width="32%" height="40%" />
</p>

# roxiP

A portable ARM-based operating system (based solely on .bin files)

> [!NOTE]
> This its a early stage, Expect errors or imperfections

## About
This is a bare-metal operating system project, designed primarily for portability.

It relies on a `.bin` file; therefore, you need a bootloader to load this file and locate the storage file.

Yes, a storage file—instead of traditional partitions, the system uses a single file to store data and other information.

This approach ensures portability, allowing you to allocate a specific amount of storage (in MB or GB).

The system is built on a YAFS (Yet Another File System)—a lightweight, custom-designed file system tailored specifically for this storage file. The goal is for it to run on various hardware, including Chinese-manufactured devices, Raspberry Pi, and other ARM-based platforms.
   
## Contrib
<table>
  <tbody>
    <tr>
      <td align="center" valign="top" width="14.28%"><img src="https://codeberg.org/avatars/6497ffe1597f5fb6406f06a4bb716218ddfbb5c99dd1fee3d48b2a947ea8be59" width="100px;" alt="PascalX"/><br /><sub><b>pascalX</b></sub></a><br /><a href="https://codeberg.org/pascalX" title="Code">😒</a></td>
      <td align="center" valign="top" width="14.28%"><img src="https://avatars.githubusercontent.com/u/167466992?v=4" width="100px;" alt="AndresDev859674"/><br /><sub><b>Andrew (AndresDev859674)</b></sub></a><br /><a href="https://github.com/AndresDev859674" title="Code">💻</a></td>
    </tr>
  </tbody>
</table>
