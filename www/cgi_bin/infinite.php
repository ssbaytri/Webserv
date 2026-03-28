<?php
echo "Content-Type: text/html\n\n";
echo "<h1>Infinite Loop Script</h1>\n";
echo "<p>Starting infinite loop...</p>\n";
flush();

// Infinite loop
while (true) {
    echo ".";
    flush();
    sleep(1);
}

echo "<p>This will never print</p>\n";
?>