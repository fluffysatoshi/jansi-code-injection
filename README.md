# What is Jansi

[Jansi](https://github.com/fusesource/jansi) is a small java library
that allows you to use ANSI escape sequences to format your console
output which works even on windows.

It is [used](https://mvnrepository.com/artifact/org.fusesource.jansi/jansi/usages) by many Java console programs.
So the scope of this exploit is quite large.

# How it works

Jansi extracts a ``.so`` library to the global temp folder to implement it's terminal features via OS-depended native code.

It actually creates to files:


1. ``jansi-{random}.so.lck`` Lock file
2. ``jansi-{random}.so`` library file


Although random is a secure random string which cannot be predicted, this approach creates a race condition between step 1 and step 2.

The exploit watches for ``jansi-*lck`` file creation in the tmp folder. Once this file is created, it prepares a world-writable ``jansi-{random}.so`` to get ahead of the Jansi Java program which itself doesn't check if this file already exists and just overwrites it (keeping it's world-writeable permissions).

Now the exploit watches for a ``CLOSE_NOWRITE`` event of the ``jansi-{random}.so`` file and replaces this file via a atomic rename of its own ``jansi.so`` file.

# Exploit Requirements

1. Jansi <= Version `2.4.0`.
   The newest release (`2.4.1`) modernized its
   File-system Interface (using `O_EXCL` to create the file in the `tmp` folder).

2. No hardened `/tmp` setup (`fs.protected_regular=0`).
   Specially secured systems distributions set this kernel parameter to 1 to
   prevent overwriting tmp-files even if the are  world-writable. 
   Check: [protected_regular¶](https://docs.kernel.org/admin-guide/sysctl/fs.html#protected-regular)

	```bash
	sysctl  fs.protected_regular
	```

	On older Distributions (like Centos 7) this parameter doesn't exist.

## State of common Linux Distributions

| Distributions   | fs.protected_regular | Exploit works on default setup |
|-----------------|----------------------|--------------------------------|
| Centos 7        | not available        | ✓                              |
| Centos Stream 8 | 0                    | ✓                              |
| Centos Stream 9 | 1                    | ✗                              |


# Example Payload

You can find a example payload on [Jansi example PoC Payload](https://github.com/fluffysatoshi/jansi/tree/PoC)

After running ` ./jansi-code-injection-poc libjansi.so`. You can test example Jinja Programs and should see this output in the first line of it's output:

```
Hello from 🐦 Fluffy Satoshi

```

You might have to run the exploit multiple times: It's a race
condition. If you get a `open: Permission denied` message the Jansi
program has "won" the race and has already written the
`jansi-{random}.so` file.
