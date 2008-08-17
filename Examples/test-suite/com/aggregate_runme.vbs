Dim aggregate, result

Set aggregate = CreateObject("aggregate.aggregate")

result = aggregate.move(aggregate.UP)

If result <> aggregate.UP Then
  WScript.Echo "UP failed"
  WScript.Quit 1
End If

result = aggregate.move(aggregate.DOWN)

If result <> aggregate.DOWN Then
  WScript.Echo "DOWN failed"
  WScript.Quit 1
End If

result = aggregate.move(aggregate.LEFT)

If result <> aggregate.LEFT Then
  WScript.Echo "LEFT failed"
  WScript.Quit 1
End If

result = aggregate.move(aggregate.RIGHT)

If result <> aggregate.RIGHT Then
  WScript.Echo "RIGHT failed"
  WScript.Quit 1
End If

Rem Confirm that move() raises an exception when the contract is violated

On Error Resume Next
result = aggregate.move(0)

Rem The last statement should have produced an error

If Err.Number = 0 Then
  WScript.Echo "0 test failed"
  WScript.Quit 1
End If
