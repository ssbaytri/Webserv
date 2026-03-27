<?php
echo "Content-Type: text/html\n\n";
?>
<h1>POST Test</h1>
<p>Content-Length: <?php echo $_SERVER['CONTENT_LENGTH'] ?? 0; ?></p>
<p>Data: <?php echo file_get_contents('php://stdin'); ?></p>