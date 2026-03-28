<?php
header("Content-Type: text/html");
http_response_code(200);
?>
<h1>PHP CGI Test</h1>
<p>Method: <?php echo $_SERVER['REQUEST_METHOD']; ?></p>
<p>Query: <?php echo $_SERVER['QUERY_STRING'] ?? 'N/A'; ?></p>
<p>Script: <?php echo $_SERVER['SCRIPT_FILENAME']; ?></p>
<hr>
<h2>All Environment Variables:</h2>
<pre>
<?php
foreach ($_SERVER as $key => $value) {
    echo "$key = $value\n";
}
?>
</pre>